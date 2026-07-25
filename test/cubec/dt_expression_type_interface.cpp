#include "cubec/expression.h"
#include "cubec/expression_type_interface.h"
#include "cubec/interface_method.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/generic_param.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_type_interface : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic anonymous interface type expressions
 * ========================================================================== */

/* Simple: interface { } */
TEST_F(dt_expression_type_interface, simple_empty) {
  const char *source = "interface { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t iface =
      (cubec_expression_type_interface_t)node;
  EXPECT_EQ(iface->generic_params, nullptr);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Interface with method: interface { func next(self): Item; } */
TEST_F(dt_expression_type_interface, with_method) {
  const char *source = "interface { func next(self): Item; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t iface =
      (cubec_expression_type_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  node_t member = (node_t)vec_get(iface->members, 0);
  EXPECT_EQ(member->kind, CUBEC_NODE_INTERFACE_METHOD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Interface with associated type and method */
TEST_F(dt_expression_type_interface, with_type_and_method) {
  const char *source = "interface { type Item; func next(self): Item; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t iface =
      (cubec_expression_type_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 2);

  /* First member: associated type */
  node_t member0 = (node_t)vec_get(iface->members, 0);
  EXPECT_EQ(member0->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  /* Second member: method */
  node_t member1 = (node_t)vec_get(iface->members, 1);
  EXPECT_EQ(member1->kind, CUBEC_NODE_INTERFACE_METHOD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic anonymous interface
 * ========================================================================== */

/* interface[T] { func get(self, idx: u64): T; } */
TEST_F(dt_expression_type_interface, generic) {
  const char *source = "interface[T] { func get(self, idx: u64): T; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t iface =
      (cubec_expression_type_interface_t)node;
  ASSERT_NE(iface->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(iface->generic_params), 1);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Multi-param generic: interface[K, V] { func map(self, key: K): V; } */
TEST_F(dt_expression_type_interface, generic_multi) {
  const char *source = "interface[K, V] { func map(self, key: K): V; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t iface =
      (cubec_expression_type_interface_t)node;
  ASSERT_NE(iface->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(iface->generic_params), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Wrapped by pointer/slice/const
 * ========================================================================== */

/* Pointer to interface: *interface { func bar(): i32; } */
TEST_F(dt_expression_type_interface, pointer_to_interface) {
  const char *source = "*interface { func bar(): i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice of interface: []interface { func run(); } */
TEST_F(dt_expression_type_interface, slice_of_interface) {
  const char *source = "[]interface { func run(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Const interface: const interface { func bar(); } */
TEST_F(dt_expression_type_interface, const_interface) {
  const char *source = "const interface { func bar(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER);

  cubec_expression_type_qualifier_t qual =
      (cubec_expression_type_qualifier_t)node;
  ASSERT_NE(qual->type, nullptr);
  EXPECT_EQ(qual->type->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_expression_type_interface, consume_all_tokens) {
  const char *source = "interface { }";
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
 *  Non-interface returns NULL
 * ========================================================================== */

TEST_F(dt_expression_type_interface, non_interface_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type_interface(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Clone and move
 * ========================================================================== */

TEST_F(dt_expression_type_interface, clone) {
  const char *source = "interface { func next(self): Item; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t copy =
      (cubec_expression_type_interface_t)cloned;
  ASSERT_NE(copy->members, nullptr);
  EXPECT_EQ(vec_get_size(copy->members), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_type_interface, move) {
  const char *source = "interface { func next(self): Item; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  cubec_expression_type_interface_t result =
      (cubec_expression_type_interface_t)moved;
  ASSERT_NE(result->members, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Via read_atom (expression context)
 * ========================================================================== */

TEST_F(dt_expression_type_interface, via_read_atom) {
  const char *source = "interface { func bar(): i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_atom(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Via read_expression (full expression pipeline) */
TEST_F(dt_expression_type_interface, via_read_expression) {
  const char *source = "interface { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPE_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
