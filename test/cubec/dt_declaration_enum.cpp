#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/declaration_enum.h"
#include "cubec/enum_item.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_qualifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_declaration_enum : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic anonymous enum type expressions
 * ========================================================================== */

/* Simple: enum { } */
TEST_F(dt_declaration_enum, simple_empty) {
  const char *source = "enum { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ENUM);

  cubec_declaration_enum_t enum_node =
      (cubec_declaration_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* enum with items: enum { Red, Green, Blue } */
TEST_F(dt_declaration_enum, with_items) {
  const char *source = "enum { Red, Green, Blue }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ENUM);

  cubec_declaration_enum_t enum_node =
      (cubec_declaration_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 3);

  /* Check first item */
  cubec_enum_item_t item0 =
      (cubec_enum_item_t)vec_get(enum_node->items, 0);
  EXPECT_EQ(item0->super.kind, CUBEC_NODE_ENUM_ITEM);
  ASSERT_NE(item0->name, nullptr);
  EXPECT_EQ(item0->type, nullptr);
  EXPECT_EQ(item0->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* enum with type and value: enum { A: u8 = 1 } */
TEST_F(dt_declaration_enum, with_type_and_value) {
  const char *source = "enum { A: u8 = 1 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ENUM);

  cubec_declaration_enum_t enum_node =
      (cubec_declaration_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 1);

  cubec_enum_item_t item0 =
      (cubec_enum_item_t)vec_get(enum_node->items, 0);
  ASSERT_NE(item0->type, nullptr);
  ASSERT_NE(item0->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Wrapped by pointer/slice/const
 * ========================================================================== */

/* Pointer to enum: *enum { A, B } */
TEST_F(dt_declaration_enum, pointer_to_enum) {
  const char *source = "*enum { A, B }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_DECLARATION_ENUM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_declaration_enum, consume_all_tokens) {
  const char *source = "enum { Red, Green }";
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
 *  Non-enum returns NULL
 * ========================================================================== */

TEST_F(dt_declaration_enum, non_enum_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_enum(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Clone and move
 * ========================================================================== */

TEST_F(dt_declaration_enum, clone) {
  const char *source = "enum { Red, Green }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_DECLARATION_ENUM);

  cubec_declaration_enum_t copy =
      (cubec_declaration_enum_t)cloned;
  ASSERT_NE(copy->items, nullptr);
  EXPECT_EQ(vec_get_size(copy->items), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_enum, move) {
  const char *source = "enum { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_DECLARATION_ENUM);

  cubec_declaration_enum_t result =
      (cubec_declaration_enum_t)moved;
  ASSERT_NE(result->items, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Via read_atom / read_expression
 * ========================================================================== */

TEST_F(dt_declaration_enum, via_read_atom) {
  const char *source = "enum { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_atom(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ENUM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_enum, via_read_expression) {
  const char *source = "enum { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_ENUM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_enum, write_empty_enum) {
  const char *source = "enum { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "enum {}");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
