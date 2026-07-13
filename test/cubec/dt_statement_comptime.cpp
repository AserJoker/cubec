#include "cubec/statement_comptime.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/statement_declaration.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_comptime : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ==========================================================================
 *  comptime block
 * ========================================================================== */

TEST_F(dt_statement_comptime, block_basic) {
  const char *source = "comptime { var x = 1; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_BLOCK);

  cubec_statement_comptime_block_t cb = (cubec_statement_comptime_block_t)node;
  ASSERT_NE(cb->body, nullptr);
  EXPECT_EQ(cb->body->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, block_empty) {
  const char *source = "comptime { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, block_clone) {
  const char *source = "comptime { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_COMPTIME_BLOCK);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, block_move) {
  const char *source = "comptime { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_COMPTIME_BLOCK);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  comptime if
 * ========================================================================== */

TEST_F(dt_statement_comptime, if_basic) {
  const char *source = "comptime if (true) { var x = 1; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  cubec_statement_comptime_if_t ci = (cubec_statement_comptime_if_t)node;
  ASSERT_NE(ci->condition, nullptr);
  ASSERT_NE(ci->then_branch, nullptr);
  EXPECT_EQ(ci->else_branch, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, if_with_else) {
  const char *source = "comptime if (x) { var a = 1; } else { var b = 2; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  cubec_statement_comptime_if_t ci = (cubec_statement_comptime_if_t)node;
  ASSERT_NE(ci->condition, nullptr);
  ASSERT_NE(ci->then_branch, nullptr);
  ASSERT_NE(ci->else_branch, nullptr);
  EXPECT_EQ(ci->else_branch->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, if_else_if) {
  const char *source = "comptime if (a) { } else if (b) { } else { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  cubec_statement_comptime_if_t ci = (cubec_statement_comptime_if_t)node;
  ASSERT_NE(ci->else_branch, nullptr);
  EXPECT_EQ(ci->else_branch->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, if_extends_condition) {
  const char *source = "comptime if (T extends Numeric) { var x = 1; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, if_clone) {
  const char *source = "comptime if (x) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, if_move) {
  const char *source = "comptime if (x) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  comptime for
 * ========================================================================== */

TEST_F(dt_statement_comptime, for_basic) {
  const char *source = "comptime for (var i = 0; i < 4; i = i + 1) { var x = i; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_FOR);

  cubec_statement_comptime_for_t cf = (cubec_statement_comptime_for_t)node;
  ASSERT_NE(cf->init, nullptr);
  ASSERT_NE(cf->condition, nullptr);
  ASSERT_NE(cf->increment, nullptr);
  ASSERT_NE(cf->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, for_infinite) {
  const char *source = "comptime for (;;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_FOR);

  cubec_statement_comptime_for_t cf = (cubec_statement_comptime_for_t)node;
  EXPECT_EQ(cf->init, nullptr);
  EXPECT_EQ(cf->condition, nullptr);
  EXPECT_EQ(cf->increment, nullptr);
  ASSERT_NE(cf->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, for_clone) {
  const char *source = "comptime for (;;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_COMPTIME_FOR);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, for_move) {
  const char *source = "comptime for (;;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_COMPTIME_FOR);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Dispatch: non-comptime returns NULL
 * ========================================================================== */

TEST_F(dt_statement_comptime, non_comptime_returns_null) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, comptime_var_not_consumed) {
  /* comptime var should NOT be consumed by read_statement_comptime —
     it falls through to declaration parser */
  const char *source = "comptime var x: i32 = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, comptime_func_not_consumed) {
  /* comptime func should NOT be consumed by read_statement_comptime —
     it falls through to function parser */
  const char *source = "comptime func foo(): void { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_comptime(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Via read_statement dispatcher
 * ========================================================================== */

TEST_F(dt_statement_comptime, via_read_statement_block) {
  const char *source = "comptime { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, via_read_statement_if) {
  const char *source = "comptime if (x) { } else { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_IF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, via_read_statement_for) {
  const char *source = "comptime for (;;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_COMPTIME_FOR);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_comptime, comptime_var_via_read_statement) {
  /* comptime var should still be parsed as declaration statement */
  const char *source = "comptime var x: i32 = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_comptime);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
