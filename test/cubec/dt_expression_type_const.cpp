#include "cubec/expression.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_type_const.h"
#include "cubec/expression_type_group.h"
#include "cubec/expression_type_ternary.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/error.h"
#include "core/string.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_const : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic const type expressions
 * -------------------------------------------------------------------------- */

/* Simple const type: const i32 */
TEST_F(dt_expression_type_const, simple) {
  const char *source = "const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Nested const: const const i32 */
TEST_F(dt_expression_type_const, nested_const) {
  const char *source = "const const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t outer =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(outer->type, nullptr);
  EXPECT_EQ(outer->type->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t inner =
      (cubec_expression_type_const_t)outer->type;
  ASSERT_NE(inner->type, nullptr);
  EXPECT_EQ(inner->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Non-const token returns NULL */
TEST_F(dt_expression_type_const, non_const_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type_const(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Const with declaration combinations
 * -------------------------------------------------------------------------- */

/* const * i32 → type_const(pointer(*i32)) */
TEST_F(dt_expression_type_const, const_pointer) {
  const char *source = "const * i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr =
      (cubec_declaration_pointer_t)const_node->type;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_FALSE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* const [] i32 → type_const(slice([]i32)) */
TEST_F(dt_expression_type_const, const_slice) {
  const char *source = "const [] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice =
      (cubec_declaration_slice_t)const_node->type;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* const [ 10 ] i32 → type_const(array([10]i32)) */
TEST_F(dt_expression_type_const, const_array) {
  const char *source = "const [ 10 ] i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_DECLARATION_ARRAY);

  cubec_declaration_array_t array =
      (cubec_declaration_array_t)const_node->type;
  ASSERT_NE(array->type, nullptr);
  EXPECT_EQ(array->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* const * const i32 → type_const(pointer(*const i32)) */
TEST_F(dt_expression_type_const, const_pointer_const) {
  const char *source = "const * const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr =
      (cubec_declaration_pointer_t)const_node->type;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_TRUE(ptr->is_const);
  EXPECT_FALSE(ptr->is_volatile);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Const with generic instantiation and member access
 * -------------------------------------------------------------------------- */

/* const Vec[i32] → type_const(generic(Vec[i32])) */
TEST_F(dt_expression_type_const, const_generic) {
  const char *source = "const Vec[ i32 ]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind,
            CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* const std::vec::Vec → type_const(namespace_access(std::vec::Vec)) */
TEST_F(dt_expression_type_const, const_member) {
  const char *source = "const std::vec::Vec";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Const with type_group (ternary wrapping)
 * -------------------------------------------------------------------------- */

/* const ( a ? b : c ) → type_const(type_group(ternary)) */
TEST_F(dt_expression_type_const, const_with_type_group_ternary) {
  const char *source = "const ( a ? b : c )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t const_node =
      (cubec_expression_type_const_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_EXPRESSION_TYPE_GROUP);

  cubec_expression_type_group_t group =
      (cubec_expression_type_group_t)const_node->type;
  ASSERT_NE(group->inner, nullptr);
  EXPECT_EQ(group->inner->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Const as ternary condition
 * -------------------------------------------------------------------------- */

/* const a ? b : c → ternary(condition=type_const(const a), ...) */
TEST_F(dt_expression_type_const, const_as_ternary_condition) {
  const char *source = "const a ? b : c";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_TERNARY);

  cubec_expression_type_ternary_t ternary =
      (cubec_expression_type_ternary_t)node;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t cond =
      (cubec_expression_type_const_t)ternary->condition;
  ASSERT_NE(cond->type, nullptr);
  EXPECT_EQ(cond->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Clone and move
 * -------------------------------------------------------------------------- */

/* Clone produces independent copy */
TEST_F(dt_expression_type_const, clone) {
  const char *source = "const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  cubec_expression_type_const_t orig = (cubec_expression_type_const_t)node;
  cubec_expression_type_const_t copy = (cubec_expression_type_const_t)cloned;
  EXPECT_NE(orig->type, copy->type);
  EXPECT_EQ(copy->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

/* Move transfers ownership */
TEST_F(dt_expression_type_const, move) {
  const char *source = "const i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPE_CONST);

  /* value_move transfers data but does NOT free the source pointer.
   * The source must be freed explicitly. */
  allocator_free(allocator, &node);

  cubec_expression_type_const_t result = (cubec_expression_type_const_t)moved;
  ASSERT_NE(result->type, nullptr);
  EXPECT_EQ(result->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}
