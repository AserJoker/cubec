#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_ternary.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/vec.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_ternary : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;

  void SetUp() override { CubecTest::SetUp(); }
};

/* --------------------------------------------------------------------------
 *  Parse a simple ternary expression: a ? b : c
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_ternary, indirect_ternary_simple) {
  const char *source = "a ? b : c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  skip_whitespace(tokens, &position);
  ASSERT_EQ(position, 0);

  /* After skipping whitespace at start, position = 0 (token "a") */
  /* read_expression will parse "a ? b : c" as a complete ternary */

  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Parse ternary with binary condition: x + a ? b : c
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_ternary, indirect_ternary_with_binary_condition) {
  const char *source = "x + a ? b : c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  skip_whitespace(tokens, &position);

  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Exception/error cases
 * -------------------------------------------------------------------------- */

/* Missing '?' → returns condition as-is (graceful fallback, no error) */
TEST_F(dt_expression_ternary, missing_question_mark_returns_condition) {
  const char *source = "a : b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  /* Should return 'a' identifier, no error */
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Missing ':' after consequent → error */
TEST_F(dt_expression_ternary, missing_colon_error) {
  const char *source = "a ? b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* Missing consequent (empty between ? and :) → error */
TEST_F(dt_expression_ternary, missing_consequent_error) {
  const char *source = "a ? : b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* Missing alternate (empty after :) → error */
TEST_F(dt_expression_ternary, missing_alternate_error) {
  const char *source = "a ? b :";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Nested and complex cases
 * -------------------------------------------------------------------------- */

/* Nested ternary: a ? (b ? c : d) : e */
TEST_F(dt_expression_ternary, nested_ternary) {
  const char *source = "a ? b ? c : d : e";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t outer = (cubec_expression_ternary_t)node;
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  /* consequent is a nested ternary */
  EXPECT_EQ(outer->consequent->kind, CUBEC_NODE_EXPRESSION_TERNARY);
  EXPECT_EQ(outer->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary with complex alternate: a ? b : c + d */
TEST_F(dt_expression_ternary, ternary_with_complex_alternate) {
  const char *source = "a ? b : c + d";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_ternary, write_ternary) {
  const char *source = "a ? b : c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "a ? b : c");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
