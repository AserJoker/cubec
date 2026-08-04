#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression_sizeof.h"
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

class dt_expression_sizeof : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Basic sizeof with identifier ---- */

TEST_F(dt_expression_sizeof, sizeof_identifier) {
  const char *source = "sizeof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)node;
  ASSERT_NE(s->expression, nullptr);
  EXPECT_EQ(s->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- sizeof with binary expression ---- */

TEST_F(dt_expression_sizeof, sizeof_binary_expression) {
  const char *source = "sizeof(a + b)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)node;
  ASSERT_NE(s->expression, nullptr);
  EXPECT_EQ(s->expression->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- sizeof with function call ---- */

TEST_F(dt_expression_sizeof, sizeof_function_call) {
  const char *source = "sizeof(foo())";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)node;
  ASSERT_NE(s->expression, nullptr);
  EXPECT_EQ(s->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- sizeof via read_expression ---- */

TEST_F(dt_expression_sizeof, via_read_expression) {
  const char *source = "sizeof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- sizeof with member access in expression: sizeof(x).field ---- */

TEST_F(dt_expression_sizeof, sizeof_member_access_in_expression) {
  const char *source = "sizeof(x).field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  ASSERT_NE(mem->host, nullptr);
  EXPECT_EQ(mem->host->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- sizeof in binary: sizeof(x) + sizeof(y) ---- */

TEST_F(dt_expression_sizeof, sizeof_in_binary) {
  const char *source = "sizeof(x) + sizeof(y)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  EXPECT_STREQ(string_get(bin->opt), "+");
  ASSERT_NE(bin->left, nullptr);
  EXPECT_EQ(bin->left->kind, CUBEC_NODE_EXPRESSION_SIZEOF);
  ASSERT_NE(bin->right, nullptr);
  EXPECT_EQ(bin->right->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_expression_sizeof, consume_all_tokens) {
  const char *source = "sizeof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, 4);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing opening parenthesis ---- */

TEST_F(dt_expression_sizeof, missing_lparen_is_error) {
  const char *source = "sizeof x";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing closing parenthesis ---- */

TEST_F(dt_expression_sizeof, missing_rparen_is_error) {
  const char *source = "sizeof(x";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Not sizeof: regular identifier ---- */

TEST_F(dt_expression_sizeof, not_sizeof_returns_null) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_expression_sizeof, clone) {
  const char *source = "sizeof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)cloned;
  ASSERT_NE(s->expression, nullptr);
  EXPECT_EQ(s->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_expression_sizeof, move) {
  const char *source = "sizeof(x)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_sizeof(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_SIZEOF);

  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)moved;
  ASSERT_NE(s->expression, nullptr);
  EXPECT_EQ(s->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_sizeof, write_sizeof) {
  const char *source = "sizeof(x)";
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
  EXPECT_STREQ(output, "sizeof(x)");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
