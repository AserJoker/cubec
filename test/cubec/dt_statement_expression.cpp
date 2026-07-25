#include "cubec/statement_expression.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_expression : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Simple identifier expression statement ---- */

TEST_F(dt_statement_expression, simple_identifier) {
  const char *source = "foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Numeric literal expression statement ---- */

TEST_F(dt_statement_expression, numeric_literal) {
  const char *source = "42;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Binary expression statement ---- */

TEST_F(dt_statement_expression, binary_expression) {
  const char *source = "a + b;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Function call expression statement ---- */

TEST_F(dt_statement_expression, function_call) {
  const char *source = "foo();";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access expression statement ---- */

TEST_F(dt_statement_expression, namespace_access_call) {
  const char *source = "std::Vec::create();";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_expression, consume_all_tokens) {
  const char *source = "foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* foo, ; → 2 tokens + EOF */
  EXPECT_EQ(position, 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing semicolon ---- */

TEST_F(dt_statement_expression, missing_semicolon_is_error) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Error: semicolon only (empty statement) returns NULL ---- */

TEST_F(dt_statement_expression, semicolon_only_returns_null) {
  const char *source = ";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  /* read_expression returns NULL for bare ';', so read_statement_expression returns NULL */
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_expression, clone) {
  const char *source = "foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)cloned;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_expression, move) {
  const char *source = "foo;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_EXPRESSION);

  cubec_statement_expression_t stmt = (cubec_statement_expression_t)moved;
  ASSERT_NE(stmt->expression, nullptr);
  EXPECT_EQ(stmt->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
