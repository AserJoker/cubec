#include "cubec/expression.h"
#include "cubec/expression_type_union.h"
#include "cubec/union_field.h"
#include "cubec/declaration_pointer.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_union : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ==========================================================================
 *  Basic anonymous union type expressions
 * ========================================================================== */

TEST_F(dt_expression_type_union, simple_empty) {
  const char *source = "union { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  cubec_expression_type_union_t union_node =
      (cubec_expression_type_union_t)node;
  EXPECT_EQ(union_node->generic_params, nullptr);
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, with_members) {
  const char *source = "union { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  cubec_expression_type_union_t union_node =
      (cubec_expression_type_union_t)node;
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, generic) {
  const char *source = "union[T] { value: T; tag: u64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  cubec_expression_type_union_t union_node =
      (cubec_expression_type_union_t)node;
  ASSERT_NE(union_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(union_node->generic_params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, pointer_to_union) {
  const char *source = "*union { value: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, consume_all_tokens) {
  const char *source = "union { value: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  skip_whitespace(tokens, &position);
  token_t remaining = (token_t)vec_get(tokens, position);
  ASSERT_NE(remaining, nullptr);
  EXPECT_EQ(token_get_kind(remaining), CUBEC_TOKEN_EOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, non_union_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type_union(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, clone) {
  const char *source = "union { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  cubec_expression_type_union_t copy =
      (cubec_expression_type_union_t)cloned;
  ASSERT_NE(copy->members, nullptr);
  EXPECT_EQ(vec_get_size(copy->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, move) {
  const char *source = "union { value: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  cubec_expression_type_union_t result =
      (cubec_expression_type_union_t)moved;
  ASSERT_NE(result->members, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, via_read_atom) {
  const char *source = "union { value: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_atom(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_union, via_read_expression) {
  const char *source = "union { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
