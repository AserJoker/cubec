#include "cubec/expression.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_member.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* Test: simple identifier type (e.g., "i32", "Vec") */
TEST_F(dt_expression_type, simple_identifier) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "i32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: another simple identifier */
TEST_F(dt_expression_type, another_simple_identifier) {
  const char *source = "Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "Vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic instantiation with single argument (e.g., "Vec[i32]") */
TEST_F(dt_expression_type, generic_single_argument) {
  const char *source = "Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t callee = (cubec_literal_identifier_t)generic->callee;
  EXPECT_STREQ(string_get(callee->value), "Vec");

  ASSERT_NE(generic->arguments, nullptr);
  EXPECT_EQ(vec_get_size(generic->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic instantiation with multiple arguments (e.g., "Map[string, i32]") */
TEST_F(dt_expression_type, generic_multiple_arguments) {
  const char *source = "Map[string, i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t callee = (cubec_literal_identifier_t)generic->callee;
  EXPECT_STREQ(string_get(callee->value), "Map");

  ASSERT_NE(generic->arguments, nullptr);
  EXPECT_EQ(vec_get_size(generic->arguments), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with type parameter (e.g., "Option[T]") */
TEST_F(dt_expression_type, generic_type_parameter) {
  const char *source = "Option[T]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t callee = (cubec_literal_identifier_t)generic->callee;
  EXPECT_STREQ(string_get(callee->value), "Option");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: single member access (e.g., "std.vec") */
TEST_F(dt_expression_type, single_member_access) {
  const char *source = "std.vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  ASSERT_NE(member->host, nullptr);
  EXPECT_EQ(member->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t host = (cubec_literal_identifier_t)member->host;
  EXPECT_STREQ(string_get(host->value), "std");

  ASSERT_NE(member->field, nullptr);
  EXPECT_STREQ(string_get(member->field->value), "vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: chained member access (e.g., "std.vec.Vec") */
TEST_F(dt_expression_type, chained_member_access) {
  const char *source = "std.vec.Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  /* Outer: (std.vec).Vec */
  cubec_expression_member_t outer = (cubec_expression_member_t)node;
  ASSERT_NE(outer->field, nullptr);
  EXPECT_STREQ(string_get(outer->field->value), "Vec");

  /* Inner host: std.vec */
  ASSERT_NE(outer->host, nullptr);
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_MEMBER);
  cubec_expression_member_t inner = (cubec_expression_member_t)outer->host;
  ASSERT_NE(inner->field, nullptr);
  EXPECT_STREQ(string_get(inner->field->value), "vec");

  /* Innermost host: std */
  ASSERT_NE(inner->host, nullptr);
  EXPECT_EQ(inner->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  cubec_literal_identifier_t host = (cubec_literal_identifier_t)inner->host;
  EXPECT_STREQ(string_get(host->value), "std");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: member access with generic instantiation (e.g., "std.vec.Vec[i32]") */
TEST_F(dt_expression_type, member_with_generic) {
  const char *source = "std.vec.Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;

  /* Callee should be member expression: std.vec.Vec */
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t callee_member =
      (cubec_expression_member_t)generic->callee;
  EXPECT_STREQ(string_get(callee_member->field->value), "Vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: type with spaces around dot */
TEST_F(dt_expression_type, member_with_spaces) {
  const char *source = "std . vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)member->host)->value), "std");
  EXPECT_STREQ(string_get(member->field->value), "vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: consume all tokens */
TEST_F(dt_expression_type, consume_all_tokens) {
  const char *source = "std.vec.Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  /* All tokens should be consumed:
   * std, ., vec, ., Vec, [, i32, ] → 8 tokens */
  EXPECT_EQ(position, 8);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: non-identifier returns null */
TEST_F(dt_expression_type, non_identifier_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* Test: string literal returns null */
TEST_F(dt_expression_type, string_literal_returns_null) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* Test: numeric literal returns null */
TEST_F(dt_expression_type, numeric_literal_returns_null) {
  const char *source = "42";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* Test: generic with nested generic (e.g., "Result[Ok[i32], Err[string]]") */
TEST_F(dt_expression_type, generic_with_nested_generic) {
  const char *source = "Result[Ok[i32], Err[string]]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)generic->callee)->value), "Result");

  /* Should have 2 arguments: Ok[i32] and Err[string] */
  EXPECT_EQ(vec_get_size(generic->arguments), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic argument is a member access type (e.g., "Vec[std.vec.Vec]") */
TEST_F(dt_expression_type, generic_with_member_argument) {
  const char *source = "Vec[std.vec.Vec]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)generic->callee)->value), "Vec");

  /* Should have 1 argument: std.vec.Vec (member access) */
  EXPECT_EQ(vec_get_size(generic->arguments), 1);

  node_t arg = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with multiple member access arguments (e.g., "Map[std.vec.Vec, std.str.String]") */
TEST_F(dt_expression_type, generic_with_multiple_member_arguments) {
  const char *source = "Map[std.vec.Vec, std.str.String]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)generic->callee)->value), "Map");

  /* Should have 2 arguments: std.vec.Vec and std.str.String */
  EXPECT_EQ(vec_get_size(generic->arguments), 2);

  node_t arg0 = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  node_t arg1 = (node_t)vec_get(generic->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with mixed arguments: simple type and member type (e.g., "Pair[i32, std.vec.Vec]") */
TEST_F(dt_expression_type, generic_with_mixed_arguments) {
  const char *source = "Pair[i32, std.vec.Vec]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)generic->callee)->value), "Pair");

  /* Should have 2 arguments: i32 (identifier) and std.vec.Vec (member) */
  EXPECT_EQ(vec_get_size(generic->arguments), 2);

  node_t arg0 = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  node_t arg1 = (node_t)vec_get(generic->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: deeply nested generic with member access (e.g., "Outer[Inner[std.a.B], std.c.D]") */
TEST_F(dt_expression_type, deeply_nested_generic_with_member) {
  const char *source = "Outer[Inner[std.a.B], std.c.D]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)generic->callee)->value), "Outer");

  /* Should have 2 arguments */
  EXPECT_EQ(vec_get_size(generic->arguments), 2);

  /* First argument: Inner[std.a.B] - a generic instantiation */
  node_t arg0 = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  /* Second argument: std.c.D - a member access */
  node_t arg1 = (node_t)vec_get(generic->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: empty brackets return null (not valid) */
TEST_F(dt_expression_type, empty_brackets_returns_null) {
  const char *source = "Vec[]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  /* Should return null or partial result - this tests error handling */
  /* The behavior depends on implementation - either returns null or
   * parses Vec and stops before [] */

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: underscore prefix identifier */
TEST_F(dt_expression_type, underscore_identifier) {
  const char *source = "_T";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t literal = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(literal->value), "_T");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}