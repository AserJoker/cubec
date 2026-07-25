#include "cubec/statement.h"
#include "cubec/statement_declaration.h"
#include "cubec/declaration_variable.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_using : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- using with type annotation ---- */

TEST_F(dt_statement_using, using_with_type) {
  const char *source = "using a:Item = .{};";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_using);
  ASSERT_NE(decl->declarator, nullptr);
  EXPECT_EQ(decl->declarator->kind, CUBEC_NODE_DECLARATION_VARIABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- using with type inference ---- */

TEST_F(dt_statement_using, using_inferred_type) {
  const char *source = "using a = .Item{};";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_using);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- export using ---- */

TEST_F(dt_statement_using, export_using) {
  const char *source = "export using a:Item = .{};";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_using);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- using disallows undefined ---- */

TEST_F(dt_statement_using, using_undefined_error) {
  const char *source = "using a:Item = undefined;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  /* Parsing succeeds — undefined is still valid syntax,
   * the error is reported by the checker, not the parser */
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_using);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- using mutually exclusive with extern ---- */

TEST_F(dt_statement_using, using_extern_conflict) {
  const char *source = "extern using a:Item;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- using mutually exclusive with builtin ---- */

TEST_F(dt_statement_using, using_builtin_conflict) {
  const char *source = "builtin using a:Item;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- using mutually exclusive with comptime ---- */

TEST_F(dt_statement_using, using_comptime_conflict) {
  const char *source = "comptime using a:Item = .{};";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}
