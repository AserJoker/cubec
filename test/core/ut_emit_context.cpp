#include "core/emit_context.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class ut_emit_context : public CubecTest {
protected:
};

/* ---- Create / Dispose ---- */

TEST_F(ut_emit_context, create_dispose) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  ASSERT_NE(ectx, nullptr);
  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- emit_keyword ---- */

TEST_F(ut_emit_context, emit_keyword) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_keyword(ectx, "var");
  ASSERT_EQ(vec_get_size(ectx->output_tokens), 1u);

  token_t tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(tok), CUBEC_TOKEN_KEYWORD);
  EXPECT_STREQ(token_get_string(tok), "var");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- emit_symbol ---- */

TEST_F(ut_emit_context, emit_symbol) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_symbol(ectx, "=");
  ASSERT_EQ(vec_get_size(ectx->output_tokens), 1u);

  token_t tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(tok), CUBEC_TOKEN_SYMBOL);
  EXPECT_STREQ(token_get_string(tok), "=");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- emit_space ---- */

TEST_F(ut_emit_context, emit_space) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_space(ectx);
  ASSERT_EQ(vec_get_size(ectx->output_tokens), 1u);

  token_t tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(tok), CUBEC_TOKEN_WHITESPACE);
  EXPECT_STREQ(token_get_string(tok), " ");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- emit_newline with indent ---- */

TEST_F(ut_emit_context, emit_newline_zero_indent) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_newline(ectx);
  ASSERT_EQ(vec_get_size(ectx->output_tokens), 1u);

  token_t tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(tok), CUBEC_TOKEN_WHITESPACE);
  EXPECT_STREQ(token_get_string(tok), "\n");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

TEST_F(ut_emit_context, emit_newline_with_indent) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_indent(ectx, 1);
  emit_newline(ectx);
  ASSERT_EQ(vec_get_size(ectx->output_tokens), 1u);

  token_t tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_STREQ(token_get_string(tok), "\n  ");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- emit_indent ---- */

TEST_F(ut_emit_context, emit_indent_changes_level) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  EXPECT_EQ(ectx->indent_level, 0);
  emit_indent(ectx, 1);
  EXPECT_EQ(ectx->indent_level, 1);
  emit_indent(ectx, -1);
  EXPECT_EQ(ectx->indent_level, 0);

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- recover_comments_to ---- */

TEST_F(ut_emit_context, recover_single_line_comment) {
  const char *source = "var /* comment */ x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  /* Find the identifier 'x' token */
  token_t x_token = nullptr;
  for (size_t i = 0; i < vec_get_size(tokens); i++) {
    token_t t = (token_t)vec_get(tokens, i);
    if (token_get_kind(t) == CUBEC_TOKEN_IDENTIFIER) {
      x_token = t;
      break;
    }
  }
  ASSERT_NE(x_token, nullptr);

  recover_comments_to(ectx, token_get_location(x_token)->begin.offset);

  /* Should have found the comment and a trailing space */
  ASSERT_GE(vec_get_size(ectx->output_tokens), 1u);
  token_t comment_tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(comment_tok), CUBEC_TOKEN_MULTILINE_COMMENT);
  EXPECT_STREQ(token_get_string(comment_tok), "/* comment */");

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

TEST_F(ut_emit_context, recover_multiline_comment) {
  const char *source = "var /* multi\nline */ x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  token_t x_token = nullptr;
  for (size_t i = 0; i < vec_get_size(tokens); i++) {
    token_t t = (token_t)vec_get(tokens, i);
    if (token_get_kind(t) == CUBEC_TOKEN_IDENTIFIER) {
      x_token = t;
      break;
    }
  }
  ASSERT_NE(x_token, nullptr);

  recover_comments_to(ectx, token_get_location(x_token)->begin.offset);

  ASSERT_GE(vec_get_size(ectx->output_tokens), 1u);
  token_t comment_tok = (token_t)vec_get(ectx->output_tokens, 0);
  EXPECT_EQ(token_get_kind(comment_tok), CUBEC_TOKEN_MULTILINE_COMMENT);

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

TEST_F(ut_emit_context, recover_no_comments) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  /* No comments in source, recover to EOF */
  token_t eof_token = nullptr;
  for (size_t i = 0; i < vec_get_size(tokens); i++) {
    token_t t = (token_t)vec_get(tokens, i);
    if (token_get_kind(t) == CUBEC_TOKEN_EOF) {
      eof_token = t;
      break;
    }
  }
  ASSERT_NE(eof_token, nullptr);

  recover_comments_to(ectx, token_get_location(eof_token)->begin.offset);
  EXPECT_EQ(vec_get_size(ectx->output_tokens), 0u);

  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

/* ---- token_writer_render ---- */

TEST_F(ut_emit_context, token_writer_render_basic) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_keyword(ectx, "var");
  emit_space(ectx);
  emit_identifier(ectx, "x");
  emit_space(ectx);
  emit_symbol(ectx, "=");
  emit_space(ectx);
  emit_numeric(ectx, "1");
  emit_symbol(ectx, ";");

  string_t output = token_writer_render(allocator, ectx->output_tokens);
  EXPECT_STREQ(string_get(output), "var x = 1;");

  allocator_free(allocator, &output);
  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

TEST_F(ut_emit_context, token_writer_render_with_newline_and_indent) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  emit_symbol(ectx, "{");
  emit_indent(ectx, 1);
  emit_newline(ectx);
  emit_keyword(ectx, "var");
  emit_space(ectx);
  emit_identifier(ectx, "x");
  emit_symbol(ectx, ";");
  emit_indent(ectx, -1);
  emit_newline(ectx);
  emit_symbol(ectx, "}");

  string_t output = token_writer_render(allocator, ectx->output_tokens);
  EXPECT_STREQ(string_get(output), "{\n  var x;\n}");

  allocator_free(allocator, &output);
  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}

TEST_F(ut_emit_context, full_roundtrip_with_comment) {
  const char *source = "var /* comment */ x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  emit_context_t ectx = emit_context_create(allocator, tokens);

  /* Find the identifier 'x' token */
  token_t x_token = nullptr;
  for (size_t i = 0; i < vec_get_size(tokens); i++) {
    token_t t = (token_t)vec_get(tokens, i);
    if (token_get_kind(t) == CUBEC_TOKEN_IDENTIFIER) {
      x_token = t;
      break;
    }
  }
  ASSERT_NE(x_token, nullptr);

  emit_keyword(ectx, "var");
  emit_space(ectx);
  recover_comments_to(ectx, token_get_location(x_token)->begin.offset);
  emit_identifier(ectx, "x");
  emit_space(ectx);
  emit_symbol(ectx, "=");
  emit_space(ectx);
  emit_numeric(ectx, "1");
  emit_symbol(ectx, ";");

  string_t output = token_writer_render(allocator, ectx->output_tokens);
  EXPECT_STREQ(string_get(output), "var /* comment */ x = 1;");

  allocator_free(allocator, &output);
  emit_context_dispose(ectx);
  allocator_free(allocator, &tokens);
}
