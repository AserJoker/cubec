#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_member.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_group : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic group parsing
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, simple_group) {
  const char *source = "(a)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)group->inner)->value), "a");

  /* Consumed all tokens: (, a, ) → 3 */
  EXPECT_EQ(position, 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, numeric_in_group) {
  const char *source = "(42)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, group_with_spaces) {
  const char *source = "( a )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)group->inner)->value), "a");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Group with binary expression inside
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, binary_inside_group) {
  const char *source = "(a + b)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)group->inner;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Group overrides precedence
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, group_overrides_precedence) {
  /* Without group: a + b * c  →  a + (b * c)
   * With group:    (a + b) * c  →  add binds first */
  const char *source = "(a + b) * c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Outer should be * */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "*");

  /* Left should be group (a + b) */
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)bin->left;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t inner = (cubec_expression_binary_t)group->inner;
  EXPECT_STREQ(string_get(inner->opt), "+");

  /* Right should be c */
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Nested groups
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, nested_groups) {
  const char *source = "((a))";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Outer group */
  cubec_expression_group_t outer = (cubec_expression_group_t)node;
  ASSERT_NE(outer->inner, nullptr);
  EXPECT_EQ(outer->inner->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Inner group wraps identifier */
  cubec_expression_group_t inner = (cubec_expression_group_t)outer->inner;
  ASSERT_NE(inner->inner, nullptr);
  EXPECT_EQ(inner->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)inner->inner)->value), "a");

  EXPECT_EQ(position, 5); /* (, (, a, ), ) → 5 */

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, deeply_nested) {
  const char *source = "(((1 + 2)))";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Walk through 3 layers of groups */
  cubec_expression_group_t g1 = (cubec_expression_group_t)node;
  ASSERT_NE(g1->inner, nullptr);
  EXPECT_EQ(g1->inner->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t g2 = (cubec_expression_group_t)g1->inner;
  ASSERT_NE(g2->inner, nullptr);
  EXPECT_EQ(g2->inner->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t g3 = (cubec_expression_group_t)g2->inner;
  ASSERT_NE(g3->inner, nullptr);
  EXPECT_EQ(g3->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Group + other constructs interaction
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, group_with_member_access) {
  /* (obj).field  →  group wraps obj, then .field applied */
  const char *source = "(obj).field";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Outer should be member access */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  ASSERT_NE(member->host, nullptr);
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_GROUP);

  ASSERT_NE(member->field, nullptr);
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, prefix_on_group) {
  /* !(a + b)  →  prefix ! applies to group (a + b) */
  const char *source = "!(a + b)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "!");
  EXPECT_EQ(bin->left, nullptr);

  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_GROUP);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, group_inside_binary) {
  /* a * (b + c)  →  group on right side of * */
  const char *source = "a * (b + c)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "*");

  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group = (cubec_expression_group_t)bin->right;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t inner = (cubec_expression_binary_t)group->inner;
  EXPECT_STREQ(string_get(inner->opt), "+");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Error cases
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_group, missing_close_paren) {
  const char *source = "(a + b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, empty_group) {
  const char *source = "()";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_group, non_group_returns_null_from_standalone) {
  /* Calling read_expression_group directly on non-'(' should return NULL */
  const char *source = "hello";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_group(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  /* position should NOT advance */
  EXPECT_EQ(position, 0);

  allocator_free(allocator, &tokens);
}
