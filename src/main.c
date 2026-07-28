#include "core/allocator.h"
#include "core/icu_data.h"
#include "engine/context.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "c/lower.h"
#include "c/c_ir.h"
#include "c/c_ir_unit.h"
#include "writer/writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static char *read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); return NULL; }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len) *out_len = n;
  return buf;
}

static int cmd_test(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: cubecc test <file>.cubec\n");
    return 1;
  }
  const char *filename = argv[2];

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

  context_check_program(ctx, program);

  /* Emit diagnostics */
  diagnostic_list_emit(ctx->diagnostics, ctx->sources);

  int total = ctx->test_count;
  int failed = ctx->test_fail_count;
  int passed = total - failed;

  fprintf(stdout, "\n%d test(s) run, %d passed, %d failed\n",
          total, passed, failed);

  int result = (failed > 0) ? 1 : 0;

  context_dispose(ctx);
  allocator_free(allocator, &program);
  allocator_free(allocator, &tokens);
  free(source);
  delete_allocator(allocator);
  return result;
}

static int cmd_build(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: cubecc build <file>.cubec [--library]\n");
    return 1;
  }
  const char *filename = argv[2];
  bool generate_executable = true;

  /* Parse options */
  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "--library") == 0) {
      generate_executable = false;
    }
  }

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

  context_check_program(ctx, program);

  /* Emit diagnostics */
  diagnostic_list_emit(ctx->diagnostics, ctx->sources);

  if (ctx->error_count > 0) {
    fprintf(stderr, "error: %d error(s) during compilation\n", ctx->error_count);
    context_dispose(ctx);
    allocator_free(allocator, &program);
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  /* Lower to C IR */
  c_ir_unit_t unit = lower_program(allocator, ctx, program, generate_executable);
  if (!unit) {
    fprintf(stderr, "error: lowering failed\n");
    context_dispose(ctx);
    allocator_free(allocator, &program);
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  /* Write C output */
  string_t out_h = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = NULL});
  string_t out_c = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = NULL});
  writer_write_unit(allocator, unit, out_h, out_c);

  /* Write .h/.c files to build/ directory */
  /* Extract the base filename (strip directory and extension) */
  const char *last_slash = strrchr(filename, '/');
  const char *last_bslash = strrchr(filename, '\\');
  const char *base_start = filename;
  if (last_slash && last_slash + 1 > base_start) base_start = last_slash + 1;
  if (last_bslash && last_bslash + 1 > base_start) base_start = last_bslash + 1;
  const char *dot = strrchr(base_start, '.');
  size_t base_len = dot ? (size_t)(dot - base_start) : strlen(base_start);

  char *h_path = (char *)malloc(6 + base_len + 3); /* "build/" + name + ".h" */
  memcpy(h_path, "build/", 6);
  memcpy(h_path + 6, base_start, base_len);
  h_path[6 + base_len] = '.';
  h_path[6 + base_len + 1] = 'h';
  h_path[6 + base_len + 2] = '\0';

  char *c_path = (char *)malloc(6 + base_len + 3);
  memcpy(c_path, "build/", 6);
  memcpy(c_path + 6, base_start, base_len);
  c_path[6 + base_len] = '.';
  c_path[6 + base_len + 1] = 'c';
  c_path[6 + base_len + 2] = '\0';

  /* Ensure build/ directory exists */
#ifdef _WIN32
  _mkdir("build");
#else
  mkdir("build", 0755);
#endif

  FILE *hf = fopen(h_path, "w");
  if (hf) { fputs(string_get(out_h), hf); fclose(hf); }
  else { fprintf(stderr, "error: cannot write '%s'\n", h_path); }

  FILE *cf = fopen(c_path, "w");
  if (cf) { fputs(string_get(out_c), cf); fclose(cf); }
  else { fprintf(stderr, "error: cannot write '%s'\n", c_path); }

  fprintf(stdout, "Generated: %s, %s\n", h_path, c_path);

  free(h_path);
  free(c_path);

  /* Cleanup */
  c_ir_node_t unit_node = (c_ir_node_t)unit;
  c_ir_dispose(allocator, &unit_node);
  allocator_free(allocator, &out_h);
  allocator_free(allocator, &out_c);
  context_dispose(ctx);
  allocator_free(allocator, &program);
  allocator_free(allocator, &tokens);
  free(source);
  delete_allocator(allocator);
  return 0;
}

int _main(int argc, char *argv[]) {
  icu_data_init();

  if (argc >= 2 && strcmp(argv[1], "test") == 0) {
    return cmd_test(argc, argv);
  }
  if (argc >= 2 && strcmp(argv[1], "build") == 0) {
    return cmd_build(argc, argv);
  }

  fprintf(stderr, "usage: cubec <command> [args]\n");
  fprintf(stderr, "  test <file>.cubec       Run test blocks\n");
  fprintf(stderr, "  build <file>.cubec      Compile to C (executable by default)\n");
  fprintf(stderr, "    --library             Generate library (no C main entry)\n");
  return 1;
}

int main(int argc, char *argv[]) {
  int res = _main(argc, argv);
  return res;
}
