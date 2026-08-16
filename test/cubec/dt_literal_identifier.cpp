#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_literal_identifier : public CubecTest {
protected:
};

TEST_F(dt_literal_identifier, parse_non_identifier_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_identifier, parse_simple_identifier) {
  const char *source = "hello";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "hello");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_identifier, parse_underscore_identifier) {
  const char *source = "_private";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "_private");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_identifier, parse_another_identifier) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "foo");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_identifier, parse_string_returns_null) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_identifier, write_foo) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_literal_identifier(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "foo");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
