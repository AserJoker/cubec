#include "cubec/statement_defer.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_defer : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_statement_defer, defer_block) {
  const char *source = "defer { file.close(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  ASSERT_NE(defer_node->body, nullptr);
  EXPECT_EQ(defer_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);
  EXPECT_EQ(defer_node->captures, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, defer_empty_block) {
  const char *source = "defer { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  EXPECT_EQ(defer_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);
  EXPECT_EQ(defer_node->captures, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, defer_with_capture) {
  const char *source = "defer |file| { close(file); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  ASSERT_NE(defer_node->body, nullptr);
  EXPECT_EQ(defer_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);
  ASSERT_NE(defer_node->captures, nullptr);
  EXPECT_EQ(vec_get_size(defer_node->captures), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, defer_with_multiple_captures) {
  const char *source = "defer |x, y| { print(x + y); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  ASSERT_NE(defer_node->captures, nullptr);
  EXPECT_EQ(vec_get_size(defer_node->captures), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, defer_empty_captures) {
  const char *source = "defer || { no_capture(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  EXPECT_EQ(defer_node->captures, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, clone) {
  const char *source = "defer |x| { print(x); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, move) {
  const char *source = "defer |x| { print(x); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, via_read_statement) {
  const char *source = "defer { cleanup(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, non_defer_returns_null) {
  const char *source = "break;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}
