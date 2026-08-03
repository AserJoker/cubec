#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_qualifier.h"
#include "cubec/declaration_pointer.h"
#include "cubec/expression_group.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/string.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_constraint : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* --------------------------------------------------------------------------
 *  Basic constraint operators (now binary ops)
 * -------------------------------------------------------------------------- */

/* extends: T extends U → EXPRESSION_BINARY with opt "extends" */
TEST_F(dt_expression_type_constraint, simple_extends) {
  const char *source = "T extends U";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==: T == U → EXPRESSION_BINARY with opt "==" */
TEST_F(dt_expression_type_constraint, simple_eq) {
  const char *source = "T == U";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "==");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* !=: T != U → EXPRESSION_BINARY with opt "!=" */
TEST_F(dt_expression_type_constraint, simple_ne) {
  const char *source = "T != U";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "!=");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Fallback: no constraint operator → returns left operand as-is
 * -------------------------------------------------------------------------- */

/* Plain identifier returns as-is */
TEST_F(dt_expression_type_constraint, fallback_simple_identifier) {
  const char *source = "a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Right operand can be complex type expressions
 * -------------------------------------------------------------------------- */

/* extends with generic right operand: T extends Vec[i32] */
TEST_F(dt_expression_type_constraint, extends_generic_right) {
  const char *source = "T extends Vec[ i32 ]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* == with namespace access right operand: T == std::vec::Vec */
TEST_F(dt_expression_type_constraint, eq_member_right) {
  const char *source = "T == std::vec::Vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "==");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* != with pointer right operand: T != * const i32
 * Note: This requires read_expression_type() because the value expression
 * parser cannot handle *-prefixed type expressions yet. */
TEST_F(dt_expression_type_constraint, ne_pointer_right) {
  const char *source = "T != * const i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "!=");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Constraint as ternary condition (via read_expression_type)
 * -------------------------------------------------------------------------- */

/* T extends U ? X : Y — binary extends as ternary condition */
TEST_F(dt_expression_type_constraint, extends_ternary) {
  const char *source = "T extends U ? X : Y";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "extends");

  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* T == U ? X : Y — equality binary as ternary condition */
TEST_F(dt_expression_type_constraint, eq_ternary) {
  const char *source = "T == i32 ? Vec[ T ] : T";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "==");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(ternary->consequent->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* T != U ? X : Y — inequality binary as ternary condition */
TEST_F(dt_expression_type_constraint, ne_ternary) {
  const char *source = "T != f64 ? f32 : T";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)ternary->condition;
  EXPECT_STREQ(string_get(bin->opt), "!=");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  typeof + extends in value context
 * -------------------------------------------------------------------------- */

/* (typeof(a) extends i32) ? 1 : 2 — value ternary with typeof+extends condition */
TEST_F(dt_expression_type_constraint, typeof_extends_ternary_value) {
  const char *source = "(typeof(a) extends i32) ? 1 : 2";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  /* Condition is a group wrapping binary(typeof, extends, i32) */
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)ternary->condition;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)group->inner;
  EXPECT_STREQ(string_get(bin->opt), "extends");
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_TYPEOF);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  /* Consequent and alternate are numeric literals */
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_NUMERIC);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_constraint, pointer_to_extends_ternary_via_group) {
  const char *source = "* ( T extends U ? X : Y )";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
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
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
