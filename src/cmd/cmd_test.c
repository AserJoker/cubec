#include "cmd/cmd_test.h"

#include "core/allocator.h"
#include "core/icu_data.h"
#include "engine/context.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include <string.h>

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

static int cmd_test_run(const cmd_parsed_t *parsed) {
  const char *filename = parsed->positional[0];

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

static const cmd_option_t test_options[] = {
  {NULL, NULL, NULL, false},
};

const cmd_subcommand_t cmd_test_def = {
  .name = "test",
  .help = "Run test blocks",
  .options = test_options,
  .option_count = 0,
  .min_positional = 1,
  .max_positional = 1,
  .run = cmd_test_run,
};
