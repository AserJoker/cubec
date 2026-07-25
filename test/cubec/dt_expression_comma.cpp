#include "cubec/expression.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_comma.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_comma : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ============================================================================
 *  Helper macros and functions
 * ============================================================================ */

/* Helper: verify a comma node's structure */
static void expect_comma(node_t node, cubec_node_kind_t left_kind,
                         cubec_node_kind_t right_kind) {
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);
  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  ASSERT_NE(comma->left, nullptr);
  EXPECT_EQ(comma->left->kind, left_kind);
  ASSERT_NE(comma->right, nullptr);
  EXPECT_EQ(comma->right->kind, right_kind);
}

/* ============================================================================
 *  Basic comma expressions: a, b
 * ============================================================================ */

TEST_F(dt_expression_comma, simple_two_elements) {
  const char *source = "a, b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_comma(node, CUBEC_NODE_LITERAL_IDENTIFIER, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, two_numeric_literals) {
  const char *source = "1, 2";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_comma(node, CUBEC_NODE_LITERAL_NUMERIC, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Chained comma expressions (right-associative): a, b, c => comma(a, comma(b, c))
 * ============================================================================ */

TEST_F(dt_expression_comma, three_identifiers_right_associative) {
  /* a, b, c should parse as comma(a, comma(b, c)) */
  const char *source = "a, b, c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t outer = (cubec_expression_comma_t)node;
  EXPECT_EQ(outer->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(outer->right->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t inner = (cubec_expression_comma_t)outer->right;
  EXPECT_EQ(inner->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(inner->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, four_identifiers_right_associative) {
  /* a, b, c, d should parse as comma(a, comma(b, comma(c, d))) */
  const char *source = "a, b, c, d";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t outer = (cubec_expression_comma_t)node;
  EXPECT_EQ(outer->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(outer->right->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t level2 = (cubec_expression_comma_t)outer->right;
  EXPECT_EQ(level2->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(level2->right->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t level3 = (cubec_expression_comma_t)level2->right;
  EXPECT_EQ(level3->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(level3->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Comma with assignment expression: a = b, c and a, b = c
 * ============================================================================ */

TEST_F(dt_expression_comma, left_is_assignment) {
  /* a = b, c should parse as comma(assignment(a, b), c) */
  const char *source = "a = b, c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  EXPECT_EQ(comma->left->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);
  EXPECT_EQ(comma->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, right_is_assignment) {
  /* a, b = c should parse as comma(a, assignment(b, c)) */
  const char *source = "a, b = c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  EXPECT_EQ(comma->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(comma->right->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Comma with other expressions
 * ============================================================================ */

TEST_F(dt_expression_comma, comma_with_binary_expression) {
  /* a + b, c */
  const char *source = "a + b, c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  EXPECT_EQ(comma->left->kind, CUBEC_NODE_EXPRESSION_BINARY);
  EXPECT_EQ(comma->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, comma_with_call_expression) {
  /* foo(), bar */
  const char *source = "foo(), bar";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_COMMA);

  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  EXPECT_EQ(comma->left->kind, CUBEC_NODE_EXPRESSION_CALL);
  EXPECT_EQ(comma->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Non-comma expressions return NULL
 * ============================================================================ */

TEST_F(dt_expression_comma, non_comma_returns_identifier) {
  /* A simple identifier without comma should return that identifier */
  const char *source = "x";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, binary_without_comma_returns_binary) {
  /* a + b without comma should return the binary expression */
  const char *source = "a + b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Whitespace handling
 * ============================================================================ */

TEST_F(dt_expression_comma, with_whitespace) {
  const char *source = "a    ,    b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_comma(node, CUBEC_NODE_LITERAL_IDENTIFIER, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_comma, no_whitespace) {
  const char *source = "a,b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_comma(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_comma(node, CUBEC_NODE_LITERAL_IDENTIFIER, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
