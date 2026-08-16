#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_alignof : public CubecTest {
protected:
};

/* ---- Basic alignof with identifier ---- */

TEST_F(dt_expression_alignof, alignof_identifier) {
  const char *source = "alignof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  cubec_expression_alignof_t a = (cubec_expression_alignof_t)node;
  ASSERT_NE(a->expression, nullptr);
  EXPECT_EQ(a->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- alignof with binary expression ---- */

TEST_F(dt_expression_alignof, alignof_binary_expression) {
  const char *source = "alignof(a + b)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_alignof_t a = (cubec_expression_alignof_t)node;
  ASSERT_NE(a->expression, nullptr);
  EXPECT_EQ(a->expression->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- alignof with function call ---- */

TEST_F(dt_expression_alignof, alignof_function_call) {
  const char *source = "alignof(foo())";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_alignof_t a = (cubec_expression_alignof_t)node;
  ASSERT_NE(a->expression, nullptr);
  EXPECT_EQ(a->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- alignof via read_expression ---- */

TEST_F(dt_expression_alignof, via_read_expression) {
  const char *source = "alignof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- alignof with member access in expression: alignof(x).field ---- */

TEST_F(dt_expression_alignof, alignof_member_access_in_expression) {
  const char *source = "alignof(x).field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  ASSERT_NE(mem->host, nullptr);
  EXPECT_EQ(mem->host->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- alignof in binary: alignof(x) + alignof(y) ---- */

TEST_F(dt_expression_alignof, alignof_in_binary) {
  const char *source = "alignof(x) + alignof(y)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_expression_alignof, consume_all_tokens) {
  const char *source = "alignof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, 4);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing opening parenthesis ---- */

TEST_F(dt_expression_alignof, missing_lparen_is_error) {
  const char *source = "alignof x";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing closing parenthesis ---- */

TEST_F(dt_expression_alignof, missing_rparen_is_error) {
  const char *source = "alignof(x";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Not alignof: regular identifier ---- */

TEST_F(dt_expression_alignof, not_alignof_returns_null) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_expression_alignof, clone) {
  const char *source = "alignof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  cubec_expression_alignof_t a = (cubec_expression_alignof_t)cloned;
  ASSERT_NE(a->expression, nullptr);
  EXPECT_EQ(a->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_expression_alignof, move) {
  const char *source = "alignof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_alignof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_ALIGNOF);

  cubec_expression_alignof_t a = (cubec_expression_alignof_t)moved;
  ASSERT_NE(a->expression, nullptr);
  EXPECT_EQ(a->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_alignof, write_alignof) {
  const char *source = "alignof(x)";
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
  EXPECT_STREQ(output, "alignof(x)");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
