#include "cmd/format.h"
#include "core/allocator.h"
#include "core/emit_context.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "engine/context.h"
#include "engine/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------
 *  File I/O helpers
 * ------------------------------------------------------------------ */

static char *read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc((size_t)len + 1);
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

static bool write_file(const char *path, const char *content, size_t len) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return false;
  size_t written = fwrite(content, 1, len, f);
  fclose(f);
  return written == len;
}

/* ------------------------------------------------------------------
 *  Options
 * ------------------------------------------------------------------ */

static const cmd_option_t format_options[] = {
    {"--check", "-c", "check if formatting is needed (no write)", false},
    {"--output", "-o", "output file path", true},
};

/* ------------------------------------------------------------------
 *  Handler
 * ------------------------------------------------------------------ */

static int format_run(const cmd_parsed_t *parsed) {
  if (parsed->positional_count < 1) {
    fprintf(stderr, "error: no input file\n");
    return 1;
  }

  const char *input_path = parsed->positional[0];
  const char *check_flag = cmd_get_option(parsed, "--check");
  const char *output_opt = cmd_get_option(parsed, "--output");

  /* 1. Read source file */
  size_t src_len = 0;
  char *source = read_file(input_path, &src_len);
  if (!source) {
    fprintf(stderr, "error: cannot read '%s'\n", input_path);
    return 1;
  }

  /* 2. Create allocator + context */
  allocator_t allocator = create_allocator(NULL, NULL);
  context_t ctx = context_create(allocator);

  /* 3. Tokenize */
  vec_t tokens = resolve_token_list(ctx, input_path, source);

  /* 4. Parse */
  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, input_path);
  if (!program) {
    fprintf(stderr, "error: failed to parse '%s'\n", input_path);
    allocator_free(allocator, &tokens);
    context_dispose(ctx);
    delete_allocator(allocator);
    free(source);
    return 1;
  }

  /* 5. Create module with compiled results */
  module_t mod = module_create(allocator, input_path, source, tokens, program);
  /* source & tokens ownership now belongs to module */

  /* 6. Emit */
  emit_context_t ectx = emit_context_create(allocator, mod->tokens);
  emit_program(ectx, mod->program);

  /* 7. Render */
  string_t output = token_writer_render(allocator, ectx->output_tokens);
  const char *output_str = string_get(output);
  size_t output_len = string_get_length(output);

  int exit_code = 0;

  if (check_flag) {
    if (output_len != src_len || memcmp(output_str, source, src_len) != 0) {
      exit_code = 1;
    }
  } else {
    const char *out_path = output_opt ? output_opt : input_path;
    if (!write_file(out_path, output_str, output_len)) {
      fprintf(stderr, "error: cannot write '%s'\n", out_path);
      exit_code = 1;
    }
  }

  /* Cleanup */
  allocator_free(allocator, &output);
  emit_context_dispose(ectx);
  module_dispose(mod);
  context_dispose(ctx);
  delete_allocator(allocator);
  return exit_code;
}

const cmd_subcommand_t cmd_format = {
    .name = "format",
    .help = "Format a .cubec source file",
    .options = format_options,
    .option_count = 2,
    .min_positional = 1,
    .max_positional = 1,
    .run = format_run,
};
