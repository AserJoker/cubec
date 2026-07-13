#include "cubec/expression.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_group.h"
#include "cubec/expression_ternary.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/error.h"
#include "core/string.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_ternary : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic ternary type expressions
 * -------------------------------------------------------------------------- */

/* Simple ternary type expression: a ? b : c */
TEST_F(dt_expression_type_ternary, simple) {
  const char *source = "a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Ternary type with group conditions
 * -------------------------------------------------------------------------- */

/* type_group condition: ( a ) ? b : c */
TEST_F(dt_expression_type_ternary, with_type_group_condition) {
  const char *source = "( a ) ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* expression_group condition (compile-time expr): ( 1 ) ? a : b */
TEST_F(dt_expression_type_ternary, with_expr_group_condition) {
  const char *source = "( 1 ) ? a : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
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

/* --------------------------------------------------------------------------
 *  Nested ternary type expressions (group prevents infinite recursion)
 * -------------------------------------------------------------------------- */

/* Nested with group boundary: ( a ? b : c ) ? d : e */
TEST_F(dt_expression_type_ternary, nested_with_group) {
  const char *source = "( a ? b : c ) ? d : e";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t outer = (cubec_expression_ternary_t)node;
  ASSERT_NE(outer->condition, nullptr);
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)outer->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t inner =
      (cubec_expression_ternary_t)group->inner;
  EXPECT_EQ(inner->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(inner->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(inner->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(outer->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(outer->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Deeply nested with groups:
 * ( a ? b : c ) ? ( d ? e : f ) : ( g ? h : i ) */
TEST_F(dt_expression_type_ternary, deeply_nested) {
  const char *source = "( a ? b : c ) ? ( d ? e : f ) : ( g ? h : i )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t outer = (cubec_expression_ternary_t)node;

  /* Condition: type_group containing inner ternary */
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Consequent: type_group containing inner ternary */
  EXPECT_EQ(outer->consequent->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Alternate: type_group containing inner ternary */
  EXPECT_EQ(outer->alternate->kind, CUBEC_NODE_EXPRESSION_GROUP);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Pointer / slice / array declarations with ternary base type
 * -------------------------------------------------------------------------- */

/* Pointer greedily consumes ternary: * a ? b : c  →  *(a ? b : c) */
TEST_F(dt_expression_type_ternary, pointer_greedy_ternary) {
  const char *source = "* a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)ptr->type;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Condition with grouped pointer: ( * a ) ? b : c */
TEST_F(dt_expression_type_ternary, with_pointer_condition) {
  const char *source = "( * a ) ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice greedily consumes ternary: [] a ? b : c  →  [](a ? b : c) */
TEST_F(dt_expression_type_ternary, slice_greedy_ternary) {
  const char *source = "[] a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)slice->type;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Array greedily consumes ternary: [ 10 ] a ? b : c  →  [10](a ? b : c) */
TEST_F(dt_expression_type_ternary, array_greedy_ternary) {
  const char *source = "[ 10 ] a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t array = (cubec_declaration_array_t)node;
  ASSERT_NE(array->type, nullptr);
  EXPECT_EQ(array->type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)array->type;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Pointer to ternary via type_group: * ( a ? b : c ) */
TEST_F(dt_expression_type_ternary, pointer_to_ternary_via_group) {
  const char *source = "* ( a ? b : c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ptr->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)group->inner;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice to ternary via type_group: [] ( a ? b : c ) */
TEST_F(dt_expression_type_ternary, slice_to_ternary_via_group) {
  const char *source = "[] ( a ? b : c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)slice->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Array to ternary via type_group: [ 10 ] ( a ? b : c ) */
TEST_F(dt_expression_type_ternary, array_to_ternary_via_group) {
  const char *source = "[ 10 ] ( a ? b : c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t array = (cubec_declaration_array_t)node;
  ASSERT_NE(array->type, nullptr);
  EXPECT_EQ(array->type->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)array->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Group vs type_group interaction in ternary branches
 * -------------------------------------------------------------------------- */

/* expression_group condition with binary expression:
 * ( a + b ) ? X : Y → condition is expression_group wrapping binary */
TEST_F(dt_expression_type_ternary, expression_group_with_binary_condition) {
  const char *source = "( a + b ) ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* type_group in consequent: a ? ( b ) : c → consequent wrapped in type_group */
TEST_F(dt_expression_type_ternary, type_group_in_consequent) {
  const char *source = "a ? ( b ) : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->consequent;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* type_group in alternate: a ? b : ( c ) → alternate wrapped in type_group */
TEST_F(dt_expression_type_ternary, type_group_in_alternate) {
  const char *source = "a ? b : ( c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->alternate;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* expression_group in consequent (value expr in type context):
 * a ? ( 1 ) : b → consequent is expression_group (1 is not a valid type) */
TEST_F(dt_expression_type_ternary, expression_group_in_consequent) {
  const char *source = "a ? ( 1 ) : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->consequent;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_NUMERIC);

  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* expression_group in alternate: a ? b : ( 1 ) */
TEST_F(dt_expression_type_ternary, expression_group_in_alternate) {
  const char *source = "a ? b : ( 1 )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->alternate;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Mixed: expression_group condition + type_group consequent + plain alternate:
 * ( a + b ) ? ( X ) : Y */
TEST_F(dt_expression_type_ternary, mixed_group_and_type_group) {
  const char *source = "( a + b ) ? ( X ) : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  /* Condition: expression_group (binary op inside parens) */
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);
  {
    cubec_expression_group_t cond =
        (cubec_expression_group_t)ternary->condition;
    ASSERT_NE(cond->inner, nullptr);
    EXPECT_EQ(cond->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);
  }

  /* Consequent: type_group (identifier inside parens is a valid type) */
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Alternate: plain identifier */
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* All three branches wrapped in groups:
 * ( a ) ? ( b ) : ( c ) — all type_groups */
TEST_F(dt_expression_type_ternary, all_branches_type_group) {
  const char *source = "( a ) ? ( b ) : ( c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_EXPRESSION_GROUP);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Verify inner contents are identifiers */
  {
    cubec_expression_group_t g =
        (cubec_expression_group_t)ternary->condition;
    EXPECT_EQ(g->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  }
  {
    cubec_expression_group_t g =
        (cubec_expression_group_t)ternary->consequent;
    EXPECT_EQ(g->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  }
  {
    cubec_expression_group_t g =
        (cubec_expression_group_t)ternary->alternate;
    EXPECT_EQ(g->inner->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  }

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Type constraint conditions (now binary ops):
 *  T extends K ? TrueType : FalseType
 * -------------------------------------------------------------------------- */

/* Basic extends constraint: T extends U ? X : Y */
TEST_F(dt_expression_type_ternary, extends_constraint_condition) {
  const char *source = "T extends U ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  /* Condition: binary "extends" */
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin =
      (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  /* Branches */
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Equality constraint: T == U ? X : Y */
TEST_F(dt_expression_type_ternary, eq_constraint_condition) {
  const char *source = "T == U ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  /* Condition: binary "==" */
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin =
      (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "==");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Inequality constraint: T != U ? X : Y */
TEST_F(dt_expression_type_ternary, ne_constraint_condition) {
  const char *source = "T != U ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  /* Condition: binary "!=" */
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin =
      (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "!=");

  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Constraint with grouped pointer right operand:
 * T extends (* U) ? X : Y */
TEST_F(dt_expression_type_ternary, constraint_right_is_pointer) {
  const char *source = "T extends (* U) ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin =
      (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  /* Right operand is type_group wrapping pointer */
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_GROUP);
  {
    cubec_expression_group_t right_group =
        (cubec_expression_group_t)bin->right;
    EXPECT_EQ(right_group->inner->kind, CUBEC_NODE_DECLARATION_POINTER);
  }

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* enable_if pattern with generic consequent:
 * T extends K ? Vec[ T ] : f32 */
TEST_F(dt_expression_type_ternary, extends_with_generic_consequent) {
  const char *source = "T extends K ? Vec[ T ] : f32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;

  /* Condition: binary "extends" */
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  /* Consequent: generic instantiation Vec[T] */
  EXPECT_EQ(ternary->consequent->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  /* Alternate: plain identifier */
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Bare constraint without '?' is a valid binary expression:
 * T extends U → returns a binary node */
TEST_F(dt_expression_type_ternary, bare_constraint_is_valid) {
  const char *source = "T extends U";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Nested ternary with constraint on outer layer:
 * ( a extends b ? c : d ) ? e : f → outer condition is type_group */
TEST_F(dt_expression_type_ternary, constraint_nested_in_group) {
  const char *source = "( a extends b ? c : d ) ? e : f";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t outer = (cubec_expression_ternary_t)node;

  /* Outer condition is type_group (because parenthesized) */
  EXPECT_EQ(outer->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  /* Inner of group: ternary with binary "extends" condition */
  cubec_expression_group_t group =
      (cubec_expression_group_t)outer->condition;
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t inner =
      (cubec_expression_ternary_t)group->inner;
  EXPECT_EQ(inner->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Error cases
 * -------------------------------------------------------------------------- */

/* Missing '?' returns condition as-is (graceful fallback) */
TEST_F(dt_expression_type_ternary, missing_question_mark) {
  const char *source = "a : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* No '?', so ternary returns condition as-is. */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  cubec_literal_identifier_t id = (cubec_literal_identifier_t)node;
  EXPECT_STREQ(string_get(id->value), "a");

  /* ': b' left in the token stream */
  EXPECT_LT(position, vec_get_size(tokens));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Missing ':' is an error */
TEST_F(dt_expression_type_ternary, missing_colon_error) {
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

/* Missing consequent is an error */
TEST_F(dt_expression_type_ternary, missing_consequent_error) {
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

/* Missing alternate is an error */
TEST_F(dt_expression_type_ternary, missing_alternate_error) {
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

/* --------------------------------------------------------------------------
 *  Complex type combinations
 * -------------------------------------------------------------------------- */

/* Generic instantiation as consequent: a ? Vec[ i32 ] : f32 */
TEST_F(dt_expression_type_ternary, consequent_generic) {
  const char *source = "a ? Vec[ i32 ] : f32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->consequent->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
