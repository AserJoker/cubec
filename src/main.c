#include "core/allocator.h"
#include "core/error.h"
#include "core/icu_data.h"
#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include <stdio.h>
#include <stdlib.h>
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

  vec_t tokens = resolve_token_list(allocator, filename, source);
  if (!tokens) {
    fprintf(stderr, "error: lexing failed\n");
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  size_t position = 0;
  node_t program = read_program_node(allocator, tokens, &position, filename);
  if (!program) {
    fprintf(stderr, "error: parsing failed\n");
    allocator_free(allocator, &tokens);
    free(source);
    delete_allocator(allocator);
    return 1;
  }

  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, filename, source, false);
  checker_check_program(ctx, program);

  /* Emit diagnostics */
  diagnostic_list_emit(ctx->diagnostics, ctx->sources);

  int total = ctx->test_count;
  int failed = ctx->test_fail_count;
  int passed = total - failed;

  fprintf(stdout, "\n%d test(s) run, %d passed, %d failed\n",
          total, passed, failed);

  int result = (failed > 0) ? 1 : 0;

  checker_dispose(ctx);
  allocator_free(allocator, &program);
  allocator_free(allocator, &tokens);
  free(source);
  delete_allocator(allocator);
  return result;
}

int _main(int argc, char *argv[]) {
  icu_data_init();

  if (argc >= 2 && strcmp(argv[1], "test") == 0) {
    return cmd_test(argc, argv);
  }

  fprintf(stderr, "usage: cubecc <command> [args]\n");
  fprintf(stderr, "  test <file>.cubec   Run test blocks\n");
  return 1;
}

int main(int argc, char *argv[]) {
  int res = _main(argc, argv);
  if (g_error) {
    char *err = error_to_string(g_error, NULL);
    fprintf(stderr, "%s", err);
    free(err);
    error_clear();
  }
  return res;
}
