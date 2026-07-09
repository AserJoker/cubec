#include "cubec/statement.h"
#include "cubec/statement_declaration.h"
#include "cubec/declaration_variable.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_declaration : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ---- Single declarator without type annotation ---- */

TEST_F(dt_statement_declaration, single_declarator_no_type) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_EQ(vec_get_size(decl->declarators), 1);

  node_t declarator = (node_t)vec_get(decl->declarators, 0);
  EXPECT_EQ(declarator->kind, CUBEC_NODE_VARIABLE_DECLARATOR);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Single declarator with type annotation ---- */

TEST_F(dt_statement_declaration, single_declarator_with_type) {
  const char *source = "var x: i32 = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_EQ(vec_get_size(decl->declarators), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Multiple declarators ---- */

TEST_F(dt_statement_declaration, multiple_declarators) {
  const char *source = "var a = 1, b = 2, c = 3;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_EQ(vec_get_size(decl->declarators), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with complex expression ---- */

TEST_F(dt_statement_declaration, complex_expression) {
  const char *source = "var name = foo() + bar();";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with pointer type ---- */

TEST_F(dt_statement_declaration, pointer_type) {
  const char *source = "var ptr: *i32 = null;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (anonymous) ---- */

TEST_F(dt_statement_declaration, initialize_list_anonymous) {
  const char *source = "var vec = .{1, 2, 3};";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (typed) ---- */

TEST_F(dt_statement_declaration, initialize_list_typed) {
  const char *source = "var vec = .Vec{1, 2, 3};";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (field items) ---- */

TEST_F(dt_statement_declaration, initialize_list_field_items) {
  const char *source = "var point = .Point[i32]{.x = 1, .y = 2};";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with nested initialize list ---- */

TEST_F(dt_statement_declaration, initialize_list_nested) {
  const char *source = "var nested = .Outer{.inner = .Inner{1, 2}};";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_declaration, consume_all_tokens) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* var, x, =, 42, ;, + whitespace/comment tokens + EOF */
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_declaration, clone) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)cloned;
  EXPECT_EQ(vec_get_size(decl->declarators), 1);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_declaration, move) {
  const char *source = "var a = 1, b = 2;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)moved;
  EXPECT_EQ(vec_get_size(decl->declarators), 2);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}