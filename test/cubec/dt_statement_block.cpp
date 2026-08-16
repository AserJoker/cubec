#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_expression.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_block : public CubecTest {
protected:
};

/* ---- Empty block: {} ---- */

TEST_F(dt_statement_block, empty_block) {
  const char *source = "{}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)node;
  EXPECT_EQ(vec_get_size(block->statements), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Block with single empty statement: {;} ---- */

TEST_F(dt_statement_block, single_empty_statement) {
  const char *source = "{ ; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)node;
  EXPECT_EQ(vec_get_size(block->statements), 1);
  EXPECT_EQ(((node_t)vec_get(block->statements, 0))->kind, CUBEC_NODE_STATEMENT_EMPTY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Block with single expression statement ---- */

TEST_F(dt_statement_block, single_expression_statement) {
  const char *source = "{ foo(); }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)node;
  EXPECT_EQ(vec_get_size(block->statements), 1);
  EXPECT_EQ(((node_t)vec_get(block->statements, 0))->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Block with multiple statements ---- */

TEST_F(dt_statement_block, multiple_statements) {
  const char *source = "{ foo(); bar; ; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)node;
  EXPECT_EQ(vec_get_size(block->statements), 3);
  EXPECT_EQ(((node_t)vec_get(block->statements, 0))->kind, CUBEC_NODE_STATEMENT_EXPRESSION);
  EXPECT_EQ(((node_t)vec_get(block->statements, 1))->kind, CUBEC_NODE_STATEMENT_EXPRESSION);
  EXPECT_EQ(((node_t)vec_get(block->statements, 2))->kind, CUBEC_NODE_STATEMENT_EMPTY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Nested blocks ---- */

TEST_F(dt_statement_block, nested_blocks) {
  const char *source = "{ { ; } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t outer = (cubec_statement_block_t)node;
  EXPECT_EQ(vec_get_size(outer->statements), 1);

  node_t inner_node = (node_t)vec_get(outer->statements, 0);
  EXPECT_EQ(inner_node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t inner = (cubec_statement_block_t)inner_node;
  EXPECT_EQ(vec_get_size(inner->statements), 1);
  EXPECT_EQ(((node_t)vec_get(inner->statements, 0))->kind, CUBEC_NODE_STATEMENT_EMPTY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Block via read_statement dispatcher ---- */

TEST_F(dt_statement_block, via_read_statement) {
  const char *source = "{ foo; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Not a block: no opening brace ---- */

TEST_F(dt_statement_block, no_brace_returns_null) {
  const char *source = "foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Error: unclosed brace ---- */

TEST_F(dt_statement_block, unclosed_brace_is_error) {
  const char *source = "{ foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  /* Error recovery: block is still returned but contains a StatementError */
  ASSERT_NE(node, nullptr);
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: unexpected token in block ---- */

TEST_F(dt_statement_block, unexpected_token_is_error) {
  const char *source = "{ 123 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  /* Error recovery: block is still returned but contains a StatementError */
  ASSERT_NE(node, nullptr);
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_block, clone) {
  const char *source = "{ foo; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)cloned;
  EXPECT_EQ(vec_get_size(block->statements), 1);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_block, move) {
  const char *source = "{ foo; bar; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_block(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_BLOCK);

  cubec_statement_block_t block = (cubec_statement_block_t)moved;
  EXPECT_EQ(vec_get_size(block->statements), 2);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_block, write_empty_block) {
  const char *source = "{}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_statement(ectx, node);
  emit_newline(ectx);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "{\n}\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
