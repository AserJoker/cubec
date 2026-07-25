#include "cubec/expression.h"
#include "cubec/expression_type_struct.h"
#include "cubec/struct_field.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/statement_declaration.h"
#include "cubec/generic_param.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_struct : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic anonymous struct type expressions
 * ========================================================================== */

/* Simple: struct { } */
TEST_F(dt_expression_type_struct, simple_empty) {
  const char *source = "struct { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  cubec_expression_type_struct_t struct_node =
      (cubec_expression_type_struct_t)node;
  EXPECT_EQ(struct_node->generic_params, nullptr);
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* struct with instance field: struct { x: i32; } */
TEST_F(dt_expression_type_struct, with_instance_field) {
  const char *source = "struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  cubec_expression_type_struct_t struct_node =
      (cubec_expression_type_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 1);

  node_t member = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(member->kind, CUBEC_NODE_STRUCT_FIELD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* struct with pub field */
TEST_F(dt_expression_type_struct, with_pub_field) {
  const char *source = "struct { pub x: i32; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_type_struct_t struct_node =
      (cubec_expression_type_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* First is pub */
  cubec_struct_field_t field0 =
      (cubec_struct_field_t)vec_get(struct_node->members, 0);
  EXPECT_TRUE(field0->is_pub);

  /* Second is private */
  cubec_struct_field_t field1 =
      (cubec_struct_field_t)vec_get(struct_node->members, 1);
  EXPECT_FALSE(field1->is_pub);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* struct with static var and instance field */
TEST_F(dt_expression_type_struct, with_static_var_and_field) {
  const char *source = "struct { var count: i32 = 0; x: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_type_struct_t struct_node =
      (cubec_expression_type_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* First is var declaration (statement_declaration wraps declaration_variable) */
  node_t member0 = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(member0->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  /* Second is struct field */
  node_t member1 = (node_t)vec_get(struct_node->members, 1);
  EXPECT_EQ(member1->kind, CUBEC_NODE_STRUCT_FIELD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic anonymous struct
 * ========================================================================== */

/* struct[T] { data: *T; } */
TEST_F(dt_expression_type_struct, generic) {
  const char *source = "struct[T] { data: *T; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  cubec_expression_type_struct_t struct_node =
      (cubec_expression_type_struct_t)node;
  ASSERT_NE(struct_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->generic_params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Wrapped by pointer/slice/const
 * ========================================================================== */

/* Pointer to struct: *struct { x: i32; } */
TEST_F(dt_expression_type_struct, pointer_to_struct) {
  const char *source = "*struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice of struct: []struct { x: i32; } */
TEST_F(dt_expression_type_struct, slice_of_struct) {
  const char *source = "[]struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Const struct: const struct { x: i32; } */
TEST_F(dt_expression_type_struct, const_struct) {
  const char *source = "const struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t qual =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(qual->type, nullptr);
  EXPECT_EQ(qual->type->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_expression_type_struct, consume_all_tokens) {
  const char *source = "struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  skip_whitespace(tokens, &position);
  token_t remaining = (token_t)vec_get(tokens, position);
  ASSERT_NE(remaining, nullptr);
  EXPECT_EQ(token_get_kind(remaining), CUBEC_TOKEN_EOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Non-struct returns NULL
 * ========================================================================== */

TEST_F(dt_expression_type_struct, non_struct_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type_struct(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Clone and move
 * ========================================================================== */

TEST_F(dt_expression_type_struct, clone) {
  const char *source = "struct { x: i32; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  cubec_expression_type_struct_t copy =
      (cubec_expression_type_struct_t)cloned;
  ASSERT_NE(copy->members, nullptr);
  EXPECT_EQ(vec_get_size(copy->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_struct, move) {
  const char *source = "struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  cubec_expression_type_struct_t result =
      (cubec_expression_type_struct_t)moved;
  ASSERT_NE(result->members, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Via read_atom / read_expression
 * ========================================================================== */

TEST_F(dt_expression_type_struct, via_read_atom) {
  const char *source = "struct { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_atom(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_struct, via_read_expression) {
  const char *source = "struct { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
