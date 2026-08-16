#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "core/token_writer.h"
#include "cubec/expression_group.h"
#include "cubec/declaration_callable.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_typeof.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/string.h"
#include "core/vec.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

class test_func_type_extends : public CubecTest {
protected:
};

/* typeof(fn) == func(i32) -> i32 鈥?binary == with function type */
TEST_F(test_func_type_extends, typeof_eq_func_type) {
  const char *source = "typeof(fn) == func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* This should parse as a binary (==), not just typeof */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_TYPEOF);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* typeof(fn) == func(i32) -> i32 ? Vec[i32] : f32
 * Greedy: func return type consumes ternary 鈫?
 * binary(typeof(fn), ==, func(i32) -> ternary(i32, Vec[i32], f32)) */
TEST_F(test_func_type_extends, typeof_eq_func_type_ternary) {
  const char *source = "typeof(fn) == func(i32) -> i32 ? Vec[i32] : f32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Greedy strategy: func return type greedily consumes ternary,
   * so the whole expression is a binary, not a ternary. */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_TYPEOF);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  /* The function type's return type is a ternary */
  cubec_declaration_callable_t func =
      (cubec_declaration_callable_t)bin->right;
  ASSERT_NE(func->return_type, nullptr);
  EXPECT_EQ(func->return_type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* typeof(fn) == (func(i32) -> i32) ? Vec[i32] : f32
 * Group disambiguation: ternary at the top level */
TEST_F(test_func_type_extends, typeof_eq_func_type_grouped_ternary) {
  const char *source = "typeof(fn) == (func(i32) -> i32) ? Vec[i32] : f32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)ternary->condition;
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_TYPEOF);
  /* Right side is a group wrapping function type */
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_GROUP);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(test_func_type_extends, write_func_type_eq) {
  const char *source = "typeof(fn) == func(i32) -> i32";
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
  EXPECT_STREQ(output, "typeof(fn) == func(i32) -> i32");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
