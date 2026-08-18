#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement_if.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_if : public CubecTest {
protected:
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

TEST_F(dt_statement_if, simple_if) {
  const char *source = "if(x > 0) { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IF);

  cubec_statement_if_t if_node = (cubec_statement_if_t)node;
  ASSERT_NE(if_node->condition, nullptr);
  ASSERT_NE(if_node->then_branch, nullptr);
  EXPECT_EQ(if_node->then_branch->kind, CUBEC_NODE_STATEMENT_BLOCK);
  EXPECT_EQ(if_node->else_branch, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_if, if_else) {
  const char *source = "if(x > 0) { } else { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_if_t if_node = (cubec_statement_if_t)node;
  ASSERT_NE(if_node->then_branch, nullptr);
  ASSERT_NE(if_node->else_branch, nullptr);
  EXPECT_EQ(if_node->else_branch->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_if, if_else_if_else) {
  const char *source = "if(x > 0) { } else if(x < 0) { } else { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_if_t if_node = (cubec_statement_if_t)node;
  ASSERT_NE(if_node->then_branch, nullptr);
  ASSERT_NE(if_node->else_branch, nullptr);
  /* else if is a nested if statement */
  EXPECT_EQ(if_node->else_branch->kind, CUBEC_NODE_STATEMENT_IF);

  cubec_statement_if_t else_if_node = (cubec_statement_if_t)if_node->else_branch;
  ASSERT_NE(else_if_node->else_branch, nullptr);
  EXPECT_EQ(else_if_node->else_branch->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

TEST_F(dt_statement_if, clone) {
  const char *source = "if(x > 0) { } else { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_IF);

  cubec_statement_if_t if_node = (cubec_statement_if_t)cloned;
  ASSERT_NE(if_node->condition, nullptr);
  ASSERT_NE(if_node->then_branch, nullptr);
  ASSERT_NE(if_node->else_branch, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_if, move) {
  const char *source = "if(x) { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_IF);

  cubec_statement_if_t if_node = (cubec_statement_if_t)moved;
  ASSERT_NE(if_node->condition, nullptr);
  ASSERT_NE(if_node->then_branch, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_statement_if, consume_all_tokens) {
  const char *source = "if(x > 0) { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  skip_whitespace(tokens, &position);
  token_t remaining = (token_t)vec_get(tokens, position);
  ASSERT_NE(remaining, nullptr);
  EXPECT_EQ(token_get_kind(remaining), CUBEC_TOKEN_EOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

TEST_F(dt_statement_if, via_read_statement) {
  const char *source = "if(x) { }";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_if, non_if_returns_null) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_if(vm, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_if, write_if_break) {
  const char *source = "if (x) break;";
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
  EXPECT_STREQ(output, "if (x) break;\n");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
