#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_binary : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ============================================================================
 *  read_expression_prefix
 * ============================================================================ */

TEST_F(dt_expression_binary, prefix_not) {
  const char *source = "!flag";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_EQ(bin->left, nullptr);
  EXPECT_STREQ(string_get(bin->opt), "!");
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)bin->right)->value), "flag");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_negate) {
  const char *source = "-42";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_EQ(bin->left, nullptr);
  EXPECT_STREQ(string_get(bin->opt), "-");
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_positive) {
  const char *source = "+value";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_address) {
  /* Postfix addr: x.& instead of &x */
  const char *source = "x.&";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_DEREF);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), ".&");
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_deref) {
  /* Postfix deref: ptr.* instead of *ptr */
  const char *source = "ptr.*";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_DEREF);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), ".*");
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_bitnot) {
  const char *source = "~mask";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "~");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_not_negate_chained) {
  const char *source = "!!x";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* Outer: !(inner) */
  cubec_expression_binary_t outer = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(outer->opt), "!");
  ASSERT_NE(outer->right, nullptr);
  EXPECT_EQ(outer->right->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* Inner: !x */
  cubec_expression_binary_t inner = (cubec_expression_binary_t)outer->right;
  EXPECT_STREQ(string_get(inner->opt), "!");
  ASSERT_NE(inner->right, nullptr);
  EXPECT_EQ(inner->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, read_value_fallback) {
  /* No prefix operator → falls back to read_value */
  const char *source = "hello";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_negate_deep_chain) {
  /* --n */
  const char *source = "--n";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_binary_t outer = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(outer->opt), "-");
  ASSERT_NE(outer->right, nullptr);
  EXPECT_EQ(outer->right->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t inner = (cubec_expression_binary_t)outer->right;
  EXPECT_STREQ(string_get(inner->opt), "-");
  ASSERT_NE(inner->right, nullptr);
  EXPECT_EQ(inner->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)inner->right)->value), "n");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_not_member) {
  /* !obj.field: ! applies to (obj.field) */
  const char *source = "!obj.field";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "!");
  /* right should be the member expression obj.field */
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, non_prefix_symbol_returns_null) {
  /* "=" is a symbol but not a prefix operator */
  const char *source = "=x";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_prefix(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

/* ============================================================================
 *  read_expression_binary — infix binary operators
 * ============================================================================ */

/* Helper: verify a binary node's operator and operand kinds */
static void expect_binary(node_t node, const char *expected_op,
                          cubec_node_kind_t left_kind,
                          cubec_node_kind_t right_kind) {
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), expected_op);
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, left_kind);
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, right_kind);
}

/* ---- multiplicative: * / % ---- */

TEST_F(dt_expression_binary, binary_multiply) {
  const char *source = "a * b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "*", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_divide) {
  const char *source = "x / y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "/", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_modulo) {
  const char *source = "a % b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "%", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- additive: + - ---- */

TEST_F(dt_expression_binary, binary_add) {
  const char *source = "a + b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "+", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_subtract) {
  const char *source = "a - b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "-", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- shift: << >> ---- */

TEST_F(dt_expression_binary, binary_left_shift) {
  const char *source = "x << 2";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "<<", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_right_shift) {
  const char *source = "x >> 3";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, ">>", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- relational: < > <= >= ---- */

TEST_F(dt_expression_binary, binary_less_than) {
  const char *source = "a < b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "<", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_greater_than) {
  const char *source = "a > b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, ">", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_less_equal) {
  const char *source = "a <= b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "<=", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_greater_equal) {
  const char *source = "a >= b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, ">=", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- equality: == != ---- */

TEST_F(dt_expression_binary, binary_equals) {
  const char *source = "a == b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "==", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_not_equals) {
  const char *source = "a != b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "!=", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- bitwise binary: & ^ | (distinct from prefix &) ---- */

TEST_F(dt_expression_binary, binary_bitwise_and) {
  const char *source = "a & b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "&", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_bitwise_xor) {
  const char *source = "a ^ b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "^", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_bitwise_or) {
  const char *source = "a | b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "|", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- logical: && || ---- */

TEST_F(dt_expression_binary, binary_logical_and) {
  const char *source = "a && b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "&&", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_logical_or) {
  const char *source = "a || b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "||", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- precedence tests ---- */

TEST_F(dt_expression_binary, precedence_mul_before_add) {
  /* a + b * c  =>  a + (b * c) */
  const char *source = "a + b * c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t rhs = (cubec_expression_binary_t)bin->right;
  EXPECT_STREQ(string_get(rhs->opt), "*");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, precedence_mul_before_add_reverse) {
  /* a * b + c  =>  (a * b) + c */
  const char *source = "a * b + c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t lhs = (cubec_expression_binary_t)bin->left;
  EXPECT_STREQ(string_get(lhs->opt), "*");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, precedence_left_assoc_add) {
  /* a + b + c  =>  (a + b) + c  (left-associative) */
  const char *source = "a + b + c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t inner = (cubec_expression_binary_t)bin->left;
  EXPECT_STREQ(string_get(inner->opt), "+");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, precedence_logical_and_or) {
  /* a && b || c  =>  (a && b) || c  (&& binds tighter) */
  const char *source = "a && b || c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "||");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t lhs = (cubec_expression_binary_t)bin->left;
  EXPECT_STREQ(string_get(lhs->opt), "&&");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, precedence_full_chain) {
  /* a * b + c < d && e | f  =>  ((((a * b) + c) < d) && (e | f)) */
  const char *source = "a * b + c < d && e | f";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* Root: && */
  cubec_expression_binary_t root = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(root->opt), "&&");

  /* Left of &&: < */
  ASSERT_NE(root->left, nullptr);
  EXPECT_EQ(root->left->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t cmp = (cubec_expression_binary_t)root->left;
  EXPECT_STREQ(string_get(cmp->opt), "<");

  /* Right of &&: | */
  ASSERT_NE(root->right, nullptr);
  EXPECT_EQ(root->right->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t bin_or = (cubec_expression_binary_t)root->right;
  EXPECT_STREQ(string_get(bin_or->opt), "|");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- prefix unary with binary: -42 * 3 => (-42) * 3 ---- */

TEST_F(dt_expression_binary, prefix_neg_then_mul) {
  /* -42 * 3  =>  (-42) * 3  (unary - binds tighter than *) */
  const char *source = "-42 * 3";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* root: * */
  cubec_expression_binary_t mul = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(mul->opt), "*");

  /* left: unary -42 */
  ASSERT_NE(mul->left, nullptr);
  EXPECT_EQ(mul->left->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t unary = (cubec_expression_binary_t)mul->left;
  EXPECT_STREQ(string_get(unary->opt), "-");
  EXPECT_EQ(unary->left, nullptr);

  /* right: 3 (numeric literal) */
  ASSERT_NE(mul->right, nullptr);
  EXPECT_EQ(mul->right->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_deref_then_add) {
  /* ptr.* + 1  =>  (ptr.*) + 1 */
  const char *source = "ptr.* + 1";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t add = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(add->opt), "+");

  /* left: ptr.* (postfix deref) */
  ASSERT_NE(add->left, nullptr);
  EXPECT_EQ(add->left->kind, CUBEC_NODE_EXPRESSION_DEREF);
  cubec_expression_binary_t deref = (cubec_expression_binary_t)add->left;
  EXPECT_STREQ(string_get(deref->opt), ".*");
  EXPECT_EQ(deref->left, nullptr);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, prefix_addr_then_binary_and) {
  /* x.& & mask  =>  (x.&) & mask  (first .& is postfix addr, second & is binary) */
  const char *source = "x.& & mask";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* root: binary & */
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "&");
  ASSERT_NE(bin->left, nullptr);
  ASSERT_NE(bin->right, nullptr);

  /* left: postfix addr x.& */
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_DEREF);
  cubec_expression_binary_t postfix = (cubec_expression_binary_t)bin->left;
  EXPECT_STREQ(string_get(postfix->opt), ".&");
  EXPECT_EQ(postfix->left, nullptr);

  /* right: mask (identifier) */
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- whitespace handling ---- */

TEST_F(dt_expression_binary, binary_no_whitespace) {
  const char *source = "a+b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "+", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_binary, binary_lots_of_whitespace) {
  const char *source = "a    +    b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  expect_binary(node, "+", CUBEC_NODE_LITERAL_IDENTIFIER,
                CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* ---- member access with binary ---- */

TEST_F(dt_expression_binary, binary_with_member_access) {
  /* obj.field + value  =>  (obj.field) + value */
  const char *source = "obj.field + value";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}
