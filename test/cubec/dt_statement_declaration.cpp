#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement.h"
#include "cubec/statement_declaration.h"
#include "cubec/declaration_variable.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_declaration : public CubecTest {
protected:
};

/* ---- Single declarator without type annotation ---- */

TEST_F(dt_statement_declaration, single_declarator_no_type) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  ASSERT_NE(decl->declarator, nullptr);
  EXPECT_EQ(decl->declarator->kind, CUBEC_NODE_DECLARATION_VARIABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Single declarator with type annotation ---- */

TEST_F(dt_statement_declaration, single_declarator_with_type) {
  const char *source = "var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  ASSERT_NE(decl->declarator, nullptr);

  cubec_declaration_variable_t dv = (cubec_declaration_variable_t)decl->declarator;
  ASSERT_NE(dv->type, nullptr);
  ASSERT_NE(dv->expression, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with complex expression ---- */

TEST_F(dt_statement_declaration, complex_expression) {
  const char *source = "var name = foo() + bar();";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with pointer type ---- */

TEST_F(dt_statement_declaration, pointer_type) {
  const char *source = "var ptr: *i32 = null;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (anonymous) ---- */

TEST_F(dt_statement_declaration, initialize_list_anonymous) {
  const char *source = "var vec = .{1, 2, 3};";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (typed) ---- */

TEST_F(dt_statement_declaration, initialize_list_typed) {
  const char *source = "var vec = .Vec{1, 2, 3};";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with initialize list expression (field items) ---- */

TEST_F(dt_statement_declaration, initialize_list_field_items) {
  const char *source = "var point = .Point[i32]{.x = 1, .y = 2};";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Declarator with nested initialize list ---- */

TEST_F(dt_statement_declaration, initialize_list_nested) {
  const char *source = "var nested = .Outer{.inner = .Inner{1, 2}};";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_declaration, consume_all_tokens) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* var, x, =, 42, ;, + whitespace/comment tokens + EOF */
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_declaration, clone) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)cloned;
  ASSERT_NE(decl->declarator, nullptr);
  EXPECT_EQ(decl->declarator->kind, CUBEC_NODE_DECLARATION_VARIABLE);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_declaration, move) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)moved;
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export single declarator ---- */

TEST_F(dt_statement_declaration, export_single_declarator) {
  const char *source = "export var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Non-export declaration ---- */

TEST_F(dt_statement_declaration, non_export_declaration) {
  const char *source = "var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_FALSE(decl->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export with type annotation ---- */

TEST_F(dt_statement_declaration, export_with_type) {
  const char *source = "export var ptr: *i32 = null;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export clone ---- */

TEST_F(dt_statement_declaration, export_clone) {
  const char *source = "export var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)cloned;
  EXPECT_TRUE(decl->is_export);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export move ---- */

TEST_F(dt_statement_declaration, export_move) {
  const char *source = "export var x = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)moved;
  EXPECT_TRUE(decl->is_export);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Extern var declaration ---- */

TEST_F(dt_statement_declaration, extern_var) {
  const char *source = "extern var errno: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_extern);
  EXPECT_FALSE(decl->is_export);
  EXPECT_FALSE(decl->is_builtin);
  ASSERT_NE(decl->declarator, nullptr);

  cubec_declaration_variable_t dv = (cubec_declaration_variable_t)decl->declarator;
  ASSERT_NE(dv->type, nullptr);
  EXPECT_EQ(dv->expression, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin var declaration ---- */

TEST_F(dt_statement_declaration, builtin_var) {
  const char *source = "builtin var VERSION: const str;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_FALSE(decl->is_export);
  EXPECT_FALSE(decl->is_extern);
  ASSERT_NE(decl->declarator, nullptr);

  cubec_declaration_variable_t dv = (cubec_declaration_variable_t)decl->declarator;
  ASSERT_NE(dv->type, nullptr);
  EXPECT_EQ(dv->expression, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export builtin var (orthogonal combination) ---- */

TEST_F(dt_statement_declaration, export_builtin_var) {
  const char *source = "export builtin var X: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_FALSE(decl->is_extern);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin export var (order-independent) ---- */

TEST_F(dt_statement_declaration, builtin_export_var) {
  const char *source = "builtin export var X: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_builtin);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: extern and export are mutually exclusive ---- */

TEST_F(dt_statement_declaration, extern_export_mutual_exclusion) {
  const char *source = "extern export var x: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: extern and builtin are mutually exclusive ---- */

TEST_F(dt_statement_declaration, extern_builtin_mutual_exclusion) {
  const char *source = "extern builtin var x: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: extern var with initializer ---- */

TEST_F(dt_statement_declaration, extern_var_with_initializer_is_error) {
  const char *source = "extern var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: builtin var with initializer ---- */

TEST_F(dt_statement_declaration, builtin_var_with_initializer_is_error) {
  const char *source = "builtin var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: extern var without type annotation ---- */

TEST_F(dt_statement_declaration, extern_var_without_type_is_error) {
  const char *source = "extern var x;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Extern var clone ---- */

TEST_F(dt_statement_declaration, extern_var_clone) {
  const char *source = "extern var errno: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)cloned;
  EXPECT_TRUE(decl->is_extern);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin var move ---- */

TEST_F(dt_statement_declaration, builtin_var_move) {
  const char *source = "builtin var VERSION: const str;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)moved;
  EXPECT_TRUE(decl->is_builtin);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Comptime declarations
 * ========================================================================== */

/* ---- Comptime var: comptime var x: i32 = 42; ---- */

TEST_F(dt_statement_declaration, comptime_var) {
  const char *source = "comptime var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_comptime);
  EXPECT_FALSE(decl->is_export);
  EXPECT_FALSE(decl->is_extern);
  EXPECT_FALSE(decl->is_builtin);
  ASSERT_NE(decl->declarator, nullptr);

  cubec_declaration_variable_t dv = (cubec_declaration_variable_t)decl->declarator;
  ASSERT_NE(dv->expression, nullptr);
  ASSERT_NE(dv->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: comptime var without initializer ---- */

TEST_F(dt_statement_declaration, comptime_var_without_initializer_is_error) {
  const char *source = "comptime var x: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Export comptime var (orthogonal combination) ---- */

TEST_F(dt_statement_declaration, export_comptime_var) {
  const char *source = "export comptime var PI: f64 = 3.14;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_comptime);
  EXPECT_FALSE(decl->is_extern);
  EXPECT_FALSE(decl->is_builtin);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Comptime export var (order-independent) ---- */

TEST_F(dt_statement_declaration, comptime_export_var) {
  const char *source = "comptime export var PI: f64 = 3.14;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_comptime);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: builtin and comptime are mutually exclusive ---- */

TEST_F(dt_statement_declaration, builtin_comptime_var_mutual_exclusion) {
  const char *source = "builtin comptime var X: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: comptime and builtin are mutually exclusive ---- */

TEST_F(dt_statement_declaration, comptime_builtin_var_mutual_exclusion) {
  const char *source = "comptime builtin var X: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: extern and comptime are mutually exclusive ---- */

TEST_F(dt_statement_declaration, extern_comptime_var_mutual_exclusion) {
  const char *source = "extern comptime var x: i32;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Comptime var clone ---- */

TEST_F(dt_statement_declaration, comptime_var_clone) {
  const char *source = "comptime var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)cloned;
  EXPECT_TRUE(decl->is_comptime);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Comptime var move ---- */

TEST_F(dt_statement_declaration, comptime_var_move) {
  const char *source = "comptime var x: i32 = 42;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)moved;
  EXPECT_TRUE(decl->is_comptime);
  ASSERT_NE(decl->declarator, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_declaration, write_var_declaration) {
  const char *source = "var x = 42;";
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
  EXPECT_STREQ(output, "var x = 42;\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
