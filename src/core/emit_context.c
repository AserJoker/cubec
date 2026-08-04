#include "core/emit_context.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/token.h"
#include <string.h>

/* --------------------------------------------------------------------------
 *  Internal: create a token and add its text to the string table
 * -------------------------------------------------------------------------- */

static token_t _create_token(emit_context_t ctx, uint32_t kind,
                             const char *text, size_t length) {
  /* Create a string_t in the string table to hold the text */
  string_t str = allocator_create(ctx->allocator, &g_string_type, NULL);
  string_nconcat(str, text, length);

  /* Create token with location pointing into the string_t buffer */
  const char *ptr = string_get(str);
  token_init_t init = {
      .kind = kind,
      .location =
          {
              .filename = NULL,
              .begin = {.line = 0, .column = 0, .offset = ptr},
              .end = {.line = 0, .column = 0, .offset = ptr + length},
          },
  };
  token_t tok = allocator_create(ctx->allocator, &g_token_type, &init);
  vec_push(ctx->string_table, str);
  vec_push(ctx->output_tokens, tok);
  return tok;
}

/* --------------------------------------------------------------------------
 *  Lifecycle
 * -------------------------------------------------------------------------- */

emit_context_t emit_context_create(allocator_t allocator, vec_t source_tokens) {
  emit_context_t ctx = allocator_alloc(allocator, sizeof(struct _emit_context_t));
  ctx->allocator = allocator;
  ctx->source_tokens = source_tokens;
  ctx->owns_source_tokens = false;
  ctx->source_token_idx = 0;
  ctx->output_tokens =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = true});
  ctx->string_table =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = true});
  ctx->indent_level = 0;
  return ctx;
}

void emit_context_dispose(emit_context_t ctx) {
  /* output_tokens and string_table are auto_dispose, freed via vec */
  allocator_free(ctx->allocator, &ctx->string_table);
  allocator_free(ctx->allocator, &ctx->output_tokens);
  if (ctx->owns_source_tokens) {
    allocator_free(ctx->allocator, &ctx->source_tokens);
  }
  allocator_free(ctx->allocator, &ctx);
}

/* --------------------------------------------------------------------------
 *  Emit helpers
 * -------------------------------------------------------------------------- */

void emit_keyword(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_KEYWORD, text, strlen(text));
}

void emit_symbol(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_SYMBOL, text, strlen(text));
}

void emit_identifier(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_IDENTIFIER, text, strlen(text));
}

void emit_numeric(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_NUMERIC, text, strlen(text));
}

void emit_string_literal(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_STRING, text, strlen(text));
}

void emit_char_literal(emit_context_t ctx, const char *text) {
  _create_token(ctx, CUBEC_TOKEN_CHAR, text, strlen(text));
}

/* --------------------------------------------------------------------------
 *  Formatting helpers
 * -------------------------------------------------------------------------- */

void emit_space(emit_context_t ctx) {
  _create_token(ctx, CUBEC_TOKEN_WHITESPACE, " ", 1);
}

void emit_newline(emit_context_t ctx) {
  /* Build "\n" + indent_level * 2 spaces */
  string_t str = allocator_create(ctx->allocator, &g_string_type, NULL);
  string_concat(str, "\n");
  for (int32_t i = 0; i < ctx->indent_level; i++) {
    string_concat(str, "  ");
  }
  const char *ptr = string_get(str);
  size_t len = string_get_length(str);

  token_init_t init = {
      .kind = CUBEC_TOKEN_WHITESPACE,
      .location =
          {
              .filename = NULL,
              .begin = {.line = 0, .column = 0, .offset = ptr},
              .end = {.line = 0, .column = 0, .offset = ptr + len},
          },
  };
  token_t tok = allocator_create(ctx->allocator, &g_token_type, &init);
  vec_push(ctx->string_table, str);
  vec_push(ctx->output_tokens, tok);
}

void emit_indent(emit_context_t ctx, int32_t delta) {
  ctx->indent_level += delta;
}

/* --------------------------------------------------------------------------
 *  Comment recovery
 * -------------------------------------------------------------------------- */

void recover_comments_to(emit_context_t ctx, const char *target_offset) {
  vec_t src = ctx->source_tokens;
  size_t *cursor = &ctx->source_token_idx;

  while (*cursor < vec_get_size(src)) {
    token_t tok = vec_get(src, *cursor);
    location_t *loc = token_get_location(tok);
    const char *tok_begin = loc->begin.offset;

    /* If this token starts at or past the target offset, stop */
    if (tok_begin >= target_offset)
      break;

    uint32_t kind = token_get_kind(tok);

    if (kind == CUBEC_TOKEN_COMMENT || kind == CUBEC_TOKEN_MULTILINE_COMMENT) {
      /* Clone the comment text into the string table and create a new token */
      const char *text = token_get_string(tok);
      size_t length = token_get_string_length(tok);
      _create_token(ctx, kind, text, length);

      /* Add a space after any comment */
      emit_space(ctx);
    }

    /* Advance past this token (whitespace, comments, errors are skipped) */
    (*cursor)++;
  }
}
