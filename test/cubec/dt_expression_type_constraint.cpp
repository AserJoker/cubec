#include "cubec/expression.h"
#include "cubec/declaration_pointer.h"
#include "cubec/expression_type_constraint.h"
#include "cubec/expression_type_group.h"
#include "cubec/expression_type_ternary.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/error.h"
#include "core/string.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_constraint : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic constraint operators
 * -------------------------------------------------------------------------- */

/* extends: T extends U */
TEST_F(dt_expression_type_constraint, simple_extends) {
  const char *source = "T extends U";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EXTENDS);
  ASSERT_NE(c->left, nullptr);
  EXPECT_EQ(c->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(c->right, nullptr);
  EXPECT_EQ(c->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==: T == U */
TEST_F(dt_expression_type_constraint, simple_eq) {
  const char *source = "T == U";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EQ);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* !=: T != U */
TEST_F(dt_expression_type_constraint, simple_ne) {
  const char *source = "T != U";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_NE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Fallback: no constraint operator → returns left operand as-is
 * -------------------------------------------------------------------------- */

/* Plain identifier returns as-is */
TEST_F(dt_expression_type_constraint, fallback_simple_identifier) {
  const char *source = "a";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EXTENDS);
  EXPECT_EQ(c->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(c->right->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* == with member access right operand: T == std.vec.Vec */
TEST_F(dt_expression_type_constraint, eq_member_right) {
  const char *source = "T == std.vec.Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EQ);
  EXPECT_EQ(c->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(c->right->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* != with pointer right operand: T != * const i32 */
TEST_F(dt_expression_type_constraint, ne_pointer_right) {
  const char *source = "T != * const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node =
      read_expression_type_constraint(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)node;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_NE);
  EXPECT_EQ(c->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(c->right->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Constraint as ternary condition (via read_expression_type)
 * -------------------------------------------------------------------------- */

/* T extends U ? X : Y — constraint as ternary condition */
TEST_F(dt_expression_type_constraint, extends_ternary) {
  const char *source = "T extends U ? X : Y";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)ternary->condition;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EXTENDS);

  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* T == U ? X : Y — equality constraint as ternary condition */
TEST_F(dt_expression_type_constraint, eq_ternary) {
  const char *source = "T == i32 ? Vec[ T ] : T";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)ternary->condition;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_EQ);
  EXPECT_EQ(c->left->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(c->right->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  EXPECT_EQ(ternary->consequent->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* T != U ? X : Y — inequality constraint as ternary condition */
TEST_F(dt_expression_type_constraint, ne_ternary) {
  const char *source = "T != f64 ? f32 : T";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  cubec_expression_type_constraint_t c =
      (cubec_expression_type_constraint_t)ternary->condition;
  EXPECT_EQ(c->op, CUBEC_TYPE_CONSTRAINT_NE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Pointer to constraint-ternary via type_group: * ( T extends U ? X : Y )
 * -------------------------------------------------------------------------- */

/* Pointer to constraint-ternary requires type_group wrapping:
 * * ( T extends U ? X : Y )  →  pointer(type_group(ternary(constraint, X, Y)))
 * Without type_group, * T extends U ? X : Y would only parse as *T
 * because pointer base_type uses read_type_expression_primary (no ternary). */
TEST_F(dt_expression_type_constraint, pointer_to_extends_ternary_via_group) {
  const char *source = "* ( T extends U ? X : Y )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group =
      (cubec_expression_type_group_t)ptr->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)group->inner;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
