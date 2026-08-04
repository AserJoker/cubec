#include "cubec/literal_char.h"
#include "cubec/node.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_literal_char : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_literal_char, parse_non_char_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_char, parse_simple_char) {
  const char *source = "'a'";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, 'a');

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_char, parse_escape_char) {
  const char *source = "'\\n'";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, '\\');

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_char, parse_digit_char) {
  const char *source = "'5'";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, '5');

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_char, parse_string_returns_null) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_char, write_simple_char) {
  const char *source = "'a'";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_literal_char(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "'a'");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}