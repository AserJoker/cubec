#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement_defer.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_defer : public CubecTest {
protected:
};

TEST_F(dt_statement_defer, defer_block) {
  const char *source = "defer { file.close(); }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  EXPECT_EQ(defer_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);
  EXPECT_EQ(defer_node->captures, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, defer_with_capture) {
  const char *source = "defer |file| { close(file); }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  EXPECT_EQ(defer_node->captures, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, clone) {
  const char *source = "defer |x| { print(x); }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, move) {
  const char *source = "defer |x| { print(x); }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, via_read_statement) {
  const char *source = "defer { cleanup(); }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DEFER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, non_defer_returns_null) {
  const char *source = "break;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_defer(vm, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_defer, write_defer_empty_block) {
  const char *source = "defer { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_statement(ectx, node);
  emit_newline(ectx);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "defer {\n}\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
