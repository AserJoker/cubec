#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_member.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_spread : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* --------------------------------------------------------------------------
 *  Basic spread parsing
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_spread, spread_identifier) {
  const char *source = "...a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)spread->value)->value), "a");

  EXPECT_EQ(position, 2); /* ..., a */

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, spread_numeric) {
  const char *source = "...42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  EXPECT_EQ(position, 2); /* ..., 42 */

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, spread_with_spaces) {
  const char *source = "... a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)spread->value)->value), "a");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Spread with compound values
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_spread, spread_member_access) {
  /* ...obj.field  →  spread wraps member access */
  const char *source = "...obj.field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member =
      (cubec_expression_member_t)spread->value;
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, spread_group) {
  /* ...(a + b)  →  spread wraps grouped binary */
  const char *source = "...(a + b)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)spread->value;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, spread_binary_value) {
  /* ...a + b  →  spread wraps the entire binary expression a + b */
  const char *source = "...a + b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)spread->value;
  EXPECT_STREQ(string_get(bin->opt), "+");

  /* ...a + b entirely consumed: ..., a, WS, +, WS, b */
  EXPECT_EQ(position, 6);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Non-spread: should return NULL without advancing position
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_spread, non_spread_returns_null) {
  const char *source = "hello";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_EQ(position, 0); /* NOT advanced */

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, single_dot_returns_null) {
  const char *source = ".a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_EQ(position, 0);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, double_dot_returns_null) {
  const char *source = "..a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_EQ(position, 0);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, spread_without_value) {
  /* ... alone is invalid */
  const char *source = "...";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_spread, dots_not_at_start) {
  /* ... is only valid as a spread prefix, and the function checks from position */
  const char *source = "a...b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  /* position at 0: expect NULL since 'a' is not '.' */
  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_EQ(position, 0);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Spread with prefix unary
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_spread, spread_with_prefix) {
  /* ...ptr.* → spread wraps dereferenced ptr */
  const char *source = "...ptr.*";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_spread(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  ASSERT_NE(spread->value, nullptr);
  /* ptr.* is a postfix deref node */
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_EXPRESSION_DEREF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
