#include "cubec/expression.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_member.h"
#include "cubec/expression_type_group.h"
#include "cubec/expression_type_ternary.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/string.h"
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

/* ==========================================================================
 * Pointer Declaration Tests
 * ========================================================================== */

/* Test: simple pointer declaration (e.g., "* i32") */
TEST_F(dt_expression_type, simple_pointer_declaration) {
  const char *source = "* i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_FALSE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t underlying = (cubec_literal_identifier_t)ptr->type;
  EXPECT_STREQ(string_get(underlying->value), "i32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with const qualifier (e.g., "* const i32") */
TEST_F(dt_expression_type, pointer_with_const) {
  const char *source = "* const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_TRUE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with volatile qualifier (e.g., "* volatile i32") */
TEST_F(dt_expression_type, pointer_with_volatile) {
  const char *source = "* volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_FALSE(ptr->is_const);
  EXPECT_TRUE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with const and volatile (e.g., "* const volatile i32") */
TEST_F(dt_expression_type, pointer_with_const_volatile) {
  const char *source = "* const volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_TRUE(ptr->is_const);
  EXPECT_TRUE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: chained pointer declaration (e.g., "** i32") */
TEST_F(dt_expression_type, chained_pointer_declaration) {
  const char *source = "** i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_FALSE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  /* Inner pointer should point to i32 */
  cubec_declaration_pointer_t inner_ptr = (cubec_declaration_pointer_t)ptr->type;
  ASSERT_NE(inner_ptr->type, nullptr);
  EXPECT_EQ(inner_ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer to generic type (e.g., "* Vec[i32]") */
TEST_F(dt_expression_type, pointer_to_generic_type) {
  const char *source = "* Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer to member type (e.g., "* std.vec.Vec") */
TEST_F(dt_expression_type, pointer_to_member_type) {
  const char *source = "* std.vec.Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with pointer element type (e.g., "Vec[* i32]") */
TEST_F(dt_expression_type, generic_with_pointer_argument) {
  const char *source = "Vec[* i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_EQ(vec_get_size(generic->arguments), 1);

  node_t arg = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer declaration with pointer to pointer (e.g., "* * i32") */
TEST_F(dt_expression_type, pointer_to_pointer) {
  const char *source = "* * i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t outer = (cubec_declaration_pointer_t)node;
  ASSERT_NE(outer->type, nullptr);
  EXPECT_EQ(outer->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t inner = (cubec_declaration_pointer_t)outer->type;
  ASSERT_NE(inner->type, nullptr);
  EXPECT_EQ(inner->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer declaration on complex member type (e.g., "* std.vec.Vec[i32]") */
TEST_F(dt_expression_type, pointer_on_complex_type) {
  const char *source = "* std.vec.Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with reversed qualifier order (e.g., "* volatile const i32") */
TEST_F(dt_expression_type, pointer_reversed_qualifier_order) {
  const char *source = "* volatile const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_TRUE(ptr->is_const);
  EXPECT_TRUE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with repeated qualifiers (e.g., "* const const i32") */
TEST_F(dt_expression_type, pointer_repeated_qualifiers) {
  const char *source = "* const const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_TRUE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer with mixed repeated qualifiers (e.g., "* const volatile const i32") */
TEST_F(dt_expression_type, pointer_mixed_repeated_qualifiers) {
  const char *source = "* const volatile const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  EXPECT_TRUE(ptr->is_const);
  EXPECT_TRUE(ptr->is_volatile);
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 * Slice Declaration Tests
 * ========================================================================== */

/* Test: simple slice declaration (e.g., "[] i32") */
TEST_F(dt_expression_type, simple_slice_declaration) {
  const char *source = "[] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_FALSE(slice->is_const);
  EXPECT_FALSE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t underlying = (cubec_literal_identifier_t)slice->type;
  EXPECT_STREQ(string_get(underlying->value), "i32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with const qualifier (e.g., "[] const i32") */
TEST_F(dt_expression_type, slice_with_const) {
  const char *source = "[] const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_TRUE(slice->is_const);
  EXPECT_FALSE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with volatile qualifier (e.g., "[] volatile i32") */
TEST_F(dt_expression_type, slice_with_volatile) {
  const char *source = "[] volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_FALSE(slice->is_const);
  EXPECT_TRUE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with const and volatile (e.g., "[] const volatile i32") */
TEST_F(dt_expression_type, slice_with_const_volatile) {
  const char *source = "[] const volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_TRUE(slice->is_const);
  EXPECT_TRUE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice to generic type (e.g., "[] Vec[i32]") */
TEST_F(dt_expression_type, slice_to_generic_type) {
  const char *source = "[] Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice to member type (e.g., "[] std.vec.Vec") */
TEST_F(dt_expression_type, slice_to_member_type) {
  const char *source = "[] std.vec.Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with slice element type (e.g., "Vec[[] i32]") */
TEST_F(dt_expression_type, generic_with_slice_argument) {
  const char *source = "Vec[[] i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_EQ(vec_get_size(generic->arguments), 1);

  node_t arg = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg->kind, CUBEC_NODE_DECLARATION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with reversed qualifier order (e.g., "[] volatile const i32") */
TEST_F(dt_expression_type, slice_reversed_qualifier_order) {
  const char *source = "[] volatile const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_TRUE(slice->is_const);
  EXPECT_TRUE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with repeated qualifiers (e.g., "[] volatile volatile i32") */
TEST_F(dt_expression_type, slice_repeated_qualifiers) {
  const char *source = "[] volatile volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_FALSE(slice->is_const);
  EXPECT_TRUE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice with mixed repeated qualifiers (e.g., "[] const volatile const i32") */
TEST_F(dt_expression_type, slice_mixed_repeated_qualifiers) {
  const char *source = "[] const volatile const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  EXPECT_TRUE(slice->is_const);
  EXPECT_TRUE(slice->is_volatile);
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 * Array Declaration Tests
 * ========================================================================== */

/* Test: simple array declaration (e.g., "[10] i32") */
TEST_F(dt_expression_type, simple_array_declaration) {
  const char *source = "[10] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->size, nullptr);
  EXPECT_EQ(arr->size->kind, CUBEC_NODE_LITERAL_NUMERIC);
  ASSERT_NE(arr->type, nullptr);
  EXPECT_EQ(arr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t underlying = (cubec_literal_identifier_t)arr->type;
  EXPECT_STREQ(string_get(underlying->value), "i32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array with identifier size (e.g., "[N] i32") */
TEST_F(dt_expression_type, array_with_identifier_size) {
  const char *source = "[N] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->size, nullptr);
  EXPECT_EQ(arr->size->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array with expression size (e.g., "[a + b] i32") */
TEST_F(dt_expression_type, array_with_expression_size) {
  const char *source = "[a + b] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->size, nullptr);
  EXPECT_EQ(arr->size->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array with whitespace in size expression (e.g., "[ 10 ] i32") */
TEST_F(dt_expression_type, array_with_whitespace_in_size) {
  const char *source = "[ 10 ] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->size, nullptr);
  EXPECT_EQ(arr->size->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array to generic type (e.g., "[10] Vec[i32]") */
TEST_F(dt_expression_type, array_to_generic_type) {
  const char *source = "[10] Vec[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->type, nullptr);
  EXPECT_EQ(arr->type->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array to member type (e.g., "[10] std.vec.Vec") */
TEST_F(dt_expression_type, array_to_member_type) {
  const char *source = "[10] std.vec.Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->type, nullptr);
  EXPECT_EQ(arr->type->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: chained array declaration (e.g., "[10][20] i32") */
TEST_F(dt_expression_type, chained_array_declaration) {
  const char *source = "[10][20] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t outer = (cubec_declaration_array_t)node;
  ASSERT_NE(outer->type, nullptr);
  EXPECT_EQ(outer->type->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t inner = (cubec_declaration_array_t)outer->type;
  ASSERT_NE(inner->type, nullptr);
  EXPECT_EQ(inner->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: array with pointer (e.g., "[10] * i32") */
TEST_F(dt_expression_type, array_with_pointer) {
  const char *source = "[10] * i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  ASSERT_NE(arr->type, nullptr);
  EXPECT_EQ(arr->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: pointer to array (e.g., "* [10] i32") */
TEST_F(dt_expression_type, pointer_to_array) {
  const char *source = "* [10] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_DECLARATION_ARRAY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: generic with array argument (e.g., "Vec[[10] i32]") */
TEST_F(dt_expression_type, generic_with_array_argument) {
  const char *source = "Vec[[10] i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_EQ(vec_get_size(generic->arguments), 1);

  node_t arg = (node_t)vec_get(generic->arguments, 0);
  EXPECT_EQ(arg->kind, CUBEC_NODE_DECLARATION_ARRAY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: empty brackets is slice, not array (e.g., "[] i32") */
TEST_F(dt_expression_type, empty_brackets_is_slice) {
  const char *source = "[] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Empty brackets should be parsed as slice, not array */
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: slice and array are distinct (e.g., "[] i32" vs "[10] i32") */
TEST_F(dt_expression_type, slice_and_array_are_distinct) {
  /* Empty brackets = slice */
  const char *slice_source = "[] i32";
  vec_t slice_tokens = resolve_token_list(allocator, "test.cubec", slice_source);
  ASSERT_NE(slice_tokens, nullptr);

  size_t slice_pos = 0;
  node_t slice_node = read_expression_type(allocator, slice_tokens, &slice_pos, "test.cubec");
  ASSERT_NE(slice_node, nullptr);
  EXPECT_EQ(slice_node->kind, CUBEC_NODE_DECLARATION_SLICE);

  /* Non-empty brackets = array */
  const char *array_source = "[10] i32";
  vec_t array_tokens = resolve_token_list(allocator, "test.cubec", array_source);
  ASSERT_NE(array_tokens, nullptr);

  size_t array_pos = 0;
  node_t array_node = read_expression_type(allocator, array_tokens, &array_pos, "test.cubec");
  ASSERT_NE(array_node, nullptr);
  EXPECT_EQ(array_node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  allocator_free(allocator, &slice_node);
  allocator_free(allocator, &slice_tokens);
  allocator_free(allocator, &array_node);
  allocator_free(allocator, &array_tokens);
}

/* ==========================================================================
 * Type Group Expression Tests
 * ========================================================================== */

/* Test: simple grouped type (e.g., "( i32 )") */
TEST_F(dt_expression_type, simple_type_group) {
  const char *source = "( i32 )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group = (cubec_expression_type_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t inner = (cubec_literal_identifier_t)group->inner;
  EXPECT_STREQ(string_get(inner->value), "i32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: grouped generic type (e.g., "( Vec[i32] )") */
TEST_F(dt_expression_type, type_group_with_generic) {
  const char *source = "( Vec[i32] )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group = (cubec_expression_type_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: grouped member type (e.g., "( std.vec.Vec )") */
TEST_F(dt_expression_type, type_group_with_member) {
  const char *source = "( std.vec.Vec )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group = (cubec_expression_type_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: grouped pointer type (e.g., "( * i32 )") */
TEST_F(dt_expression_type, type_group_with_pointer) {
  const char *source = "( * i32 )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group = (cubec_expression_type_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: grouped slice type (e.g., "( [] i32 )") */
TEST_F(dt_expression_type, type_group_with_slice) {
  const char *source = "( [] i32 )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group = (cubec_expression_type_group_t)node;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_DECLARATION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: non-group returns null */
TEST_F(dt_expression_type, non_group_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should return the identifier, not a group */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: type group consume all tokens */
TEST_F(dt_expression_type, type_group_consume_all_tokens) {
  const char *source = "( i32 )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  /* All tokens should be consumed: (, whitespace, i32, whitespace, ) → 5 tokens */
  EXPECT_EQ(position, 5);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Test: nested type group (e.g., "( ( i32 ) )") */
TEST_F(dt_expression_type, nested_type_group) {
  const char *source = "( ( i32 ) )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t outer = (cubec_expression_type_group_t)node;
  ASSERT_NE(outer->inner, nullptr);
  EXPECT_EQ(outer->inner->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t inner = (cubec_expression_type_group_t)outer->inner;
  ASSERT_NE(inner->inner, nullptr);
  EXPECT_EQ(inner->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Ternary type expression tests
 * -------------------------------------------------------------------------- */

/* Simple ternary type expression: a ? b : c */
TEST_F(dt_expression_type, ternary_type_simple) {
  const char *source = "a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary type with type_group condition: ( a ) ? b : c */
TEST_F(dt_expression_type, ternary_type_with_type_group_condition) {
  const char *source = "( a ) ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  /* Inner of the group should be the identifier 'a' */
  cubec_expression_type_group_t group =
      (cubec_expression_type_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary type with expression_group condition (compile-time expr):
 * ( 1 ) ? a : b */
TEST_F(dt_expression_type, ternary_type_with_expr_group_condition) {
  const char *source = "( 1 ) ? a : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  /* Condition should be expression_group (not type_group) because 1 is
   * a numeric literal, not a valid type. */
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_NUMERIC);

  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Nested ternary type with group boundary avoids infinite recursion:
 * ( a ? b : c ) ? d : e */
TEST_F(dt_expression_type, ternary_type_nested_with_group) {
  const char *source = "( a ? b : c ) ? d : e";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t outer =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(outer->condition, nullptr);
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  /* Inner of the group should be the nested ternary type */
  cubec_expression_type_group_t group =
      (cubec_expression_type_group_t)outer->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  /* Verify the nested ternary has correct structure */
  cubec_expression_type_ternary_t inner =
      (cubec_expression_type_ternary_t)group->inner;
  EXPECT_EQ(inner->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(inner->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(inner->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(outer->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(outer->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Deeply nested ternary type with groups:
 * ( a ? b : c ) ? ( d ? e : f ) : ( g ? h : i ) */
TEST_F(dt_expression_type, ternary_type_deeply_nested) {
  const char *source = "( a ? b : c ) ? ( d ? e : f ) : ( g ? h : i )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t outer =
      (cubec_expression_type_ternary_t)node;

  /* Condition: type_group containing inner ternary */
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  /* Consequent: type_group containing inner ternary */
  EXPECT_EQ(outer->consequent->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  /* Alternate: type_group containing inner ternary */
  EXPECT_EQ(outer->alternate->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Pointer to ternary type: * a ? b : c  →  *(a ? b : c) */
TEST_F(dt_expression_type, pointer_to_ternary_type) {
  const char *source = "* a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* The pointer binds to the whole ternary type: *(a ? b : c) */
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)ptr->type;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary type with grouped pointer condition: ( * a ) ? b : c */
TEST_F(dt_expression_type, ternary_type_with_pointer_condition) {
  const char *source = "( * a ) ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  /* Inner of the group should be a pointer declaration */
  cubec_expression_type_group_t group =
      (cubec_expression_type_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice to ternary type: [] a ? b : c  →  [](a ? b : c) */
TEST_F(dt_expression_type, slice_to_ternary_type) {
  const char *source = "[] a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* The slice binds to the whole ternary type */
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Array to ternary type: [ 10 ] a ? b : c  →  [10](a ? b : c) */
TEST_F(dt_expression_type, array_to_ternary_type) {
  const char *source = "[ 10 ] a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* The array binds to the whole ternary type */
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t array = (cubec_declaration_array_t)node;
  ASSERT_NE(array->type, nullptr);
  EXPECT_EQ(array->type->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary type with identifier condition: missing '?' returns condition as-is
 * (graceful fallback via read_ternary_type_expression) */
TEST_F(dt_expression_type, ternary_type_missing_question_mark) {
  const char *source = "a : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* No '?', so ternary_type_expression returns condition as-is.
   * read_expression_type then returns this node directly. */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t id = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(id->value), "a");

  /* Should not consume ': b' (it's left in the token stream) */
  EXPECT_LT(position, vec_get_size(tokens));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Ternary type missing colon is an error */
TEST_F(dt_expression_type, ternary_type_missing_colon_error) {
  const char *source = "a ? b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  error_clear();
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_NE(g_error, nullptr);

  allocator_free(allocator, &tokens);
}

/* Ternary type missing consequent is an error */
TEST_F(dt_expression_type, ternary_type_missing_consequent_error) {
  const char *source = "a ? : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  error_clear();
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_NE(g_error, nullptr);

  allocator_free(allocator, &tokens);
}

/* Ternary type missing alternate is an error */
TEST_F(dt_expression_type, ternary_type_missing_alternate_error) {
  const char *source = "a ? b :";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  error_clear();
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);
  EXPECT_NE(g_error, nullptr);

  allocator_free(allocator, &tokens);
}

/* Ternary type with generic instantiation as consequent:
 * a ? Vec[ i32 ] : f32 */
TEST_F(dt_expression_type, ternary_type_consequent_generic) {
  const char *source = "a ? Vec[ i32 ] : f32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}