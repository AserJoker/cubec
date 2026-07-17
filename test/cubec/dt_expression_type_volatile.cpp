#include "cubec/expression.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_group.h"
#include "cubec/expression_ternary.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/error.h"
#include "core/string.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_volatile : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic volatile type expressions
 * -------------------------------------------------------------------------- */

/* Simple volatile type: volatile i32 */
TEST_F(dt_expression_type_volatile, simple) {
  const char *source = "volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  EXPECT_FALSE(volatile_node->is_const);
  EXPECT_TRUE(volatile_node->is_volatile);
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Repeated volatile: volatile volatile i32 — duplicate volatile is merged */
TEST_F(dt_expression_type_volatile, nested_volatile) {
  const char *source = "volatile volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t q =
      (cubec_expression_type_qualifier_t)node;
  EXPECT_FALSE(q->is_const);
  EXPECT_TRUE(q->is_volatile);
  ASSERT_NE(q->type, nullptr);
  EXPECT_EQ(q->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Non-volatile token returns NULL */
TEST_F(dt_expression_type_volatile, non_volatile_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type_qualifier(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Volatile with declaration combinations
 * -------------------------------------------------------------------------- */

/* volatile * i32 → type_volatile(pointer(*i32)) */
TEST_F(dt_expression_type_volatile, volatile_pointer) {
  const char *source = "volatile * i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr =
      (cubec_declaration_pointer_t)volatile_node->type;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_FALSE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* volatile [] i32 → type_volatile(slice([]i32)) */
TEST_F(dt_expression_type_volatile, volatile_slice) {
  const char *source = "volatile [] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_DECLARATION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* volatile [ 10 ] i32 → type_volatile(array([10]i32)) */
TEST_F(dt_expression_type_volatile, volatile_array) {
  const char *source = "volatile [ 10 ] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_DECLARATION_ARRAY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* volatile * volatile i32 → type_volatile(pointer(*volatile i32)) */
TEST_F(dt_expression_type_volatile, volatile_pointer_volatile) {
  const char *source = "volatile * volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr =
      (cubec_declaration_pointer_t)volatile_node->type;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_FALSE(ptr->is_const);
  EXPECT_TRUE(ptr->is_volatile);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Volatile with generic instantiation and member access
 * -------------------------------------------------------------------------- */

/* volatile Vec[i32] → type_volatile(generic(Vec[i32])) */
TEST_F(dt_expression_type_volatile, volatile_generic) {
  const char *source = "volatile Vec[ i32 ]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* volatile std::vec::Vec → type_volatile(namespace_access(std::vec::Vec)) */
TEST_F(dt_expression_type_volatile, volatile_member) {
  const char *source = "volatile std::vec::Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Volatile with type_group (ternary wrapping)
 * -------------------------------------------------------------------------- */

/* volatile ( a ? b : c ) → type_volatile(type_group(ternary)) */
TEST_F(dt_expression_type_volatile, volatile_with_type_group_ternary) {
  const char *source = "volatile ( a ? b : c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_EXPRESSION_GROUP);

  cubec_expression_group_t group =
      (cubec_expression_group_t)volatile_node->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Volatile as ternary condition
 * -------------------------------------------------------------------------- */

/* volatile greedily consumes ternary: volatile a ? b : c → volatile(a ? b : c) */
TEST_F(dt_expression_type_volatile, volatile_greedy_ternary) {
  const char *source = "volatile a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)volatile_node->type;
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
 *  Volatile combined with const
 * -------------------------------------------------------------------------- */

/* const volatile i32 → type_const(type_volatile(i32)) */
TEST_F(dt_expression_type_volatile, const_volatile) {
  const char *source = "const volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t const_node =
      (cubec_expression_type_qualifier_t)node;
  EXPECT_TRUE(const_node->is_const);
  EXPECT_FALSE(const_node->is_volatile);
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)const_node->type;
  EXPECT_FALSE(volatile_node->is_const);
  EXPECT_TRUE(volatile_node->is_volatile);
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* volatile const i32 → type_volatile(type_const(i32)) */
TEST_F(dt_expression_type_volatile, volatile_const) {
  const char *source = "volatile const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t volatile_node =
      (cubec_expression_type_qualifier_t)node;
  EXPECT_FALSE(volatile_node->is_const);
  EXPECT_TRUE(volatile_node->is_volatile);
  ASSERT_NE(volatile_node->type, nullptr);
  EXPECT_EQ(volatile_node->type->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t const_node =
      (cubec_expression_type_qualifier_t)volatile_node->type;
  EXPECT_TRUE(const_node->is_const);
  EXPECT_FALSE(const_node->is_volatile);
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Clone and move
 * -------------------------------------------------------------------------- */

/* Clone produces independent copy */
TEST_F(dt_expression_type_volatile, clone) {
  const char *source = "volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t orig = (cubec_expression_type_qualifier_t)node;
  cubec_expression_type_qualifier_t copy = (cubec_expression_type_qualifier_t)cloned;
  EXPECT_NE(orig->type, copy->type);
  EXPECT_EQ(copy->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

/* Move transfers ownership */
TEST_F(dt_expression_type_volatile, move) {
  const char *source = "volatile i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  /* value_move transfers data but does NOT free the source pointer.
   * The source must be freed explicitly. */
  allocator_free(allocator, &node);

  cubec_expression_type_qualifier_t result = (cubec_expression_type_qualifier_t)moved;
  ASSERT_NE(result->type, nullptr);
  EXPECT_EQ(result->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}
