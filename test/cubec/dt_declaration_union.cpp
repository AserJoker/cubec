#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/declaration_union.h"
#include "cubec/union_field.h"
#include "cubec/declaration_pointer.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_declaration_union : public CubecTest {
protected:
};

/* ==========================================================================
 *  Basic anonymous union type expressions
 * ========================================================================== */

TEST_F(dt_declaration_union, simple_empty) {
  const char *source = "union { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_UNION);

  cubec_declaration_union_t union_node =
      (cubec_declaration_union_t)node;
  EXPECT_EQ(union_node->generic_params, nullptr);
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, with_members) {
  const char *source = "union { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_UNION);

  cubec_declaration_union_t union_node =
      (cubec_declaration_union_t)node;
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, generic) {
  const char *source = "union[T] { value: T; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_UNION);

  cubec_declaration_union_t union_node =
      (cubec_declaration_union_t)node;
  ASSERT_NE(union_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(union_node->generic_params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, pointer_to_union) {
  const char *source = "*union { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_DECLARATION_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, consume_all_tokens) {
  const char *source = "union { value: i32; }";
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

TEST_F(dt_declaration_union, non_union_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_union(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, clone) {
  const char *source = "union { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_DECLARATION_UNION);

  cubec_declaration_union_t copy =
      (cubec_declaration_union_t)cloned;
  ASSERT_NE(copy->members, nullptr);
  EXPECT_EQ(vec_get_size(copy->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, move) {
  const char *source = "union { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_DECLARATION_UNION);

  cubec_declaration_union_t result =
      (cubec_declaration_union_t)moved;
  ASSERT_NE(result->members, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, via_read_atom) {
  const char *source = "union { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_atom(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, via_read_expression) {
  const char *source = "union { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_union, write_empty_union) {
  const char *source = "union { }";
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
  EXPECT_STREQ(output, "union {}");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
