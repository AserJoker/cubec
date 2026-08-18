#include "cubec/expression.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_binary.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_assignment : public CubecTest {
protected:
};

/* ============================================================================
 *  Helper macros and functions
 * ============================================================================ */

/* Helper: verify an assignment node's structure */
static void expect_assignment(node_t node, const char *expected_op,
                              cubec_node_kind_t lvalue_kind,
                              cubec_node_kind_t rvalue_kind) {
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);
  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), expected_op);
  ASSERT_NE(assign->left, nullptr);
  EXPECT_EQ(assign->left->kind, lvalue_kind);
  ASSERT_NE(assign->right, nullptr);
  EXPECT_EQ(assign->right->kind, rvalue_kind);
}

/* ============================================================================
 *  Simple assignment: =
 * ============================================================================ */

TEST_F(dt_expression_assignment, simple_assignment_identifier) {
  const char *source = "x = 42";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, simple_assignment_identifier_to_identifier) {
  const char *source = "a = b";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, simple_assignment_with_whitespace) {
  const char *source = "x    =    42";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Compound assignment: += -= *= /= %= &= |= ^= <<= >>=
 * ============================================================================ */

TEST_F(dt_expression_assignment, compound_add_assignment) {
  const char *source = "x += 1";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "+=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_sub_assignment) {
  const char *source = "x -= y";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "-=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_mul_assignment) {
  const char *source = "x *= 2";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "*=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_div_assignment) {
  const char *source = "x /= y";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "/=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_mod_assignment) {
  const char *source = "x %= 10";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "%=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_and_assignment) {
  const char *source = "x &= mask";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "&=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_or_assignment) {
  const char *source = "x |= 0xFF";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "|=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_xor_assignment) {
  const char *source = "x ^= key";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "^=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_left_shift_assignment) {
  const char *source = "x <<= 4";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "<<=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_right_shift_assignment) {
  const char *source = "x >>= 2";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, ">>=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_logical_and_assignment) {
  const char *source = "x &&= y";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "&&=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, compound_logical_or_assignment) {
  const char *source = "x ||= true";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  expect_assignment(node, "||=", CUBEC_NODE_LITERAL_IDENTIFIER,
                    CUBEC_NODE_LITERAL_BOOL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Non-assignment: value without assignment operator returns NULL
 * ============================================================================ */

TEST_F(dt_expression_assignment, non_assignment_returns_null) {
  /* When no assignment operator follows, return NULL to let caller
   * try other expression types. The lvalue may be part of a larger
   * expression (e.g., "a + b" should be parsed as binary, not partial). */
  const char *source = "x + y";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, non_assignment_returns_null_for_simple_value) {
  /* A simple identifier without assignment operator returns NULL.
   * Position is not advanced since this is not a valid assignment. */
  const char *source = "x";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Assignment with complex lvalue (member access, call, slice, etc.)
 * ============================================================================ */

TEST_F(dt_expression_assignment, assignment_with_member_lvalue) {
  const char *source = "obj.field = value";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_EXPRESSION_MEMBER);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_call_lvalue) {
  /* cache.get(key) = value */
  const char *source = "cache.get(key) = value";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_EXPRESSION_CALL);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_slice_lvalue) {
  /* arr[0:5] = value */
  const char *source = "arr[0:5] = x";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_EXPRESSION_SLICE);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Assignment with complex rvalue (binary, ternary, call, etc.)
 * ============================================================================ */

TEST_F(dt_expression_assignment, assignment_with_binary_rvalue) {
  const char *source = "x = a + b";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t rhs = (cubec_expression_binary_t)assign->right;
  EXPECT_STREQ(string_get(rhs->opt), "+");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_ternary_rvalue) {
  const char *source = "x = a ? b : c";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_call_rvalue) {
  const char *source = "x = get_value()";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Assignment operator precedence (assignment binds looser than other ops)
 * ============================================================================ */

TEST_F(dt_expression_assignment, assignment_with_prefix_unary_rvalue) {
  /* x = -y  =>  x = (-y) */
  const char *source = "x = -y";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t rhs = (cubec_expression_binary_t)assign->right;
  EXPECT_STREQ(string_get(rhs->opt), "-");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_postfix_deref_rvalue) {
  /* x = ptr.*  =>  x = (ptr.*) */
  const char *source = "x = ptr.*";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_EXPRESSION_DEREF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Error cases
 * ============================================================================ */

TEST_F(dt_expression_assignment, missing_rvalue_error) {
  const char *source = "x =";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, missing_rvalue_with_compound_error) {
  const char *source = "x +=";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ============================================================================
 *  Edge cases: value parsing handles postfix operators before assignment check
 * ============================================================================ */

TEST_F(dt_expression_assignment, assignment_with_chained_postfix) {
  /* obj.field.member = value */
  const char *source = "obj.field.member = value";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_EXPRESSION_MEMBER);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, assignment_with_generic_instantiation_lvalue) {
  /* map[str] = value */
  const char *source = "map[str] = value";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position,
                                           "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ASSIGNMENT);

  cubec_expression_assignment_t assign = (cubec_expression_assignment_t)node;
  EXPECT_STREQ(string_get(assign->opt), "=");
  EXPECT_EQ(assign->left->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);
  EXPECT_EQ(assign->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_assignment, write_simple_assignment) {
  const char *source = "a = b";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression_assignment(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "a = b");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
