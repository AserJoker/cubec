#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ==========================================================================
 *  for statement
 * ========================================================================== */

class dt_statement_for : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_statement_for, simple_for) {
  const char *source = "for(var i = 0; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOR);

  cubec_statement_for_t for_node = (cubec_statement_for_t)node;
  ASSERT_NE(for_node->init, nullptr);
  ASSERT_NE(for_node->condition, nullptr);
  ASSERT_NE(for_node->increment, nullptr);
  ASSERT_NE(for_node->body, nullptr);
  EXPECT_EQ(for_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, infinite_for) {
  const char *source = "for(;;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOR);

  cubec_statement_for_t for_node = (cubec_statement_for_t)node;
  EXPECT_EQ(for_node->init, nullptr);
  EXPECT_EQ(for_node->condition, nullptr);
  EXPECT_EQ(for_node->increment, nullptr);
  ASSERT_NE(for_node->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, for_no_init) {
  const char *source = "for(; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_for_t for_node = (cubec_statement_for_t)node;
  EXPECT_EQ(for_node->init, nullptr);
  ASSERT_NE(for_node->condition, nullptr);
  ASSERT_NE(for_node->increment, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, for_no_condition) {
  const char *source = "for(var i = 0;; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_for_t for_node = (cubec_statement_for_t)node;
  ASSERT_NE(for_node->init, nullptr);
  EXPECT_EQ(for_node->condition, nullptr);
  ASSERT_NE(for_node->increment, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, for_no_increment) {
  const char *source = "for(var i = 0; i < 10;) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_for_t for_node = (cubec_statement_for_t)node;
  ASSERT_NE(for_node->init, nullptr);
  ASSERT_NE(for_node->condition, nullptr);
  EXPECT_EQ(for_node->increment, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, clone) {
  const char *source = "for(var i = 0; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_FOR);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, move) {
  const char *source = "for(var i = 0; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_FOR);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, via_read_statement) {
  const char *source = "for(var i = 0; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOR);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_for, non_for_returns_null) {
  const char *source = "while(x) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_for(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  foreach statement
 * ========================================================================== */

class dt_statement_foreach : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_statement_foreach, simple_foreach) {
  const char *source = "foreach(item: items) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOREACH);

  cubec_statement_foreach_t fe_node = (cubec_statement_foreach_t)node;
  EXPECT_FALSE(fe_node->is_const);
  ASSERT_NE(fe_node->name, nullptr);
  ASSERT_NE(fe_node->iterator, nullptr);
  ASSERT_NE(fe_node->body, nullptr);
  EXPECT_EQ(fe_node->body->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, const_foreach) {
  const char *source = "foreach(const item: items) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOREACH);

  cubec_statement_foreach_t fe_node = (cubec_statement_foreach_t)node;
  EXPECT_TRUE(fe_node->is_const);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, foreach_with_expression_iterator) {
  const char *source = "foreach(x: getItems()) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_foreach_t fe_node = (cubec_statement_foreach_t)node;
  ASSERT_NE(fe_node->iterator, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, clone) {
  const char *source = "foreach(item: items) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_FOREACH);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, move) {
  const char *source = "foreach(item: items) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_FOREACH);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, via_read_statement) {
  const char *source = "foreach(const item: items) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FOREACH);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_foreach, non_foreach_returns_null) {
  const char *source = "for(var i = 0; i < 10; i = i + 1) { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_foreach(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}
