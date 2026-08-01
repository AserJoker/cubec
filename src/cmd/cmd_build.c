#include "cmd/cmd_build.h"

#include "builder/cubec_writer.h"
#include "core/allocator.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "engine/context.h"
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static char *read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len)
    *out_len = n;
  return buf;
}

/* Extract base filename (strip directory and extension) */
static char *extract_base_name(const char *filename) {
  const char *last_slash = strrchr(filename, '/');
  const char *last_bslash = strrchr(filename, '\\');
  const char *base_start = filename;
  if (last_slash && last_slash + 1 > base_start)
    base_start = last_slash + 1;
  if (last_bslash && last_bslash + 1 > base_start)
    base_start = last_bslash + 1;
  const char *dot = strrchr(base_start, '.');
  size_t base_len = dot ? (size_t)(dot - base_start) : strlen(base_start);
  char *name = (char *)malloc(base_len + 1);
  memcpy(name, base_start, base_len);
  name[base_len] = '\0';
  return name;
}

/* Ensure build directory exists */
static void ensure_build_dir(const char *build_dir) {
#ifdef _WIN32
  _mkdir(build_dir);
#else
  mkdir(build_dir, 0755);
#endif
}

/* Write string content to file, returns 0 on success */
static int write_file(const char *path, const char *content) {
  FILE *f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "error: cannot write '%s'\n", path);
    return 1;
  }
  fputs(content, f);
  fclose(f);
  return 0;
}

/* Build path: build_dir/name.ext */
static char *build_path(const char *build_dir, const char *name,
                        const char *ext) {
  size_t dlen = strlen(build_dir);
  size_t nlen = strlen(name);
  size_t elen = strlen(ext);
  char *p = (char *)malloc(dlen + 1 + nlen + elen + 1);
  memcpy(p, build_dir, dlen);
  p[dlen] = '/';
  memcpy(p + dlen + 1, name, nlen);
  memcpy(p + dlen + 1 + nlen, ext, elen + 1);
  return p;
}

static int cmd_build_run(const cmd_parsed_t *parsed) {
  const char *filename = parsed->positional[0];
  bool build_library = cmd_get_option(parsed, "--library") != NULL;

  size_t src_len;
  char *source = read_file(filename, &src_len);
  if (!source) {
    fprintf(stderr, "error: cannot read '%s'\n", filename);
    return 1;
  }

  allocator_t allocator = create_allocator(NULL, NULL);
  context_t ctx = context_create(allocator);
  ctx->current_file = filename;
  source_cache_load(ctx->sources, filename, source, false);

  vec_t tokens = resolve_token_list(ctx, filename, source);
  if (!tokens) {
    fprintf(stderr, "error: lexing failed\n");
    context_dispose(ctx);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  size_t position = 0;
  node_t program = read_program_node(ctx, tokens, &position, filename);
  if (!program) {
    fprintf(stderr, "error: parsing failed\n");
    context_dispose(ctx);
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  /* --dump-ast: serialize AST back to Cubec source (before checks) */
  if (cmd_get_option(parsed, "--dump-ast") != NULL) {
    context_dispose(ctx);
    allocator_free(allocator, &program);
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 0;
  }

  context_check_program(ctx, program);
  diagnostic_list_emit(ctx->diagnostics, ctx->sources);

  if (ctx->error_count > 0) {
    fprintf(stderr, "error: %d error(s) during compilation\n",
            ctx->error_count);
    context_dispose(ctx);
    allocator_free(allocator, &program);
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  /* TODO: C backend not yet implemented.
   * The desugar pass (checker_desugar) + new C lower will be added here. */
  fprintf(stdout, "semantic check passed (no backend yet)\n");

  /* Generate cubec interface file for library builds */
  if (build_library) {
    string_t iface = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = NULL});
    cubec_write_interface(allocator, program, iface);
    const char *build_dir = "out";
    ensure_build_dir(build_dir);
    char *base_name = extract_base_name(filename);
    char *cubec_path = build_path(build_dir, base_name, ".cubec");
    if (write_file(cubec_path, string_get(iface)) == 0) {
      fprintf(stdout, "Generated: %s\n", cubec_path);
    }
    free(cubec_path);
    free(base_name);
    allocator_free(allocator, &iface);
  }

  context_dispose(ctx);
  allocator_free(allocator, &program);
  allocator_free(allocator, &tokens);
  free(source);
  delete_allocator(allocator);
  return 0;
}

static const cmd_option_t build_options[] = {
    {"--library", "-l", "Generate library (no C main entry)", false},
    {"--dump-ast", NULL, "Dump desugared AST as Cubec source to stdout", false},
    {NULL, NULL, NULL, false},
};

const cmd_subcommand_t cmd_build_def = {
    .name = "build",
    .help = "Compile to C and build executable/library",
    .options = build_options,
    .option_count = 2,
    .min_positional = 1,
    .max_positional = 1,
    .run = cmd_build_run,
};
