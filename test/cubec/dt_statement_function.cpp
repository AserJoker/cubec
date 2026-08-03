#include "core/string.h"
#include "core/writer.h"
#include "cubec/statement.h"
#include "cubec/statement_function.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_function : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

/* ---- Basic function: func add(a: i32, b: i32): i32 { } ---- */

TEST_F(dt_statement_function, basic_function) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_FALSE(fn->is_export);
  EXPECT_FALSE(fn->is_inline);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_c_variadic);
  EXPECT_NE(fn->name, nullptr);
  EXPECT_EQ(fn->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(fn->generic_params, nullptr);
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 2);
  EXPECT_NE(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- No params: func hello() { } ---- */

TEST_F(dt_statement_function, no_params) {
  const char *source = "func hello() { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 0);
  EXPECT_EQ(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- No return type (implicit void): func greet(name: *u8) { } ---- */

TEST_F(dt_statement_function, no_return_type) {
  const char *source = "func greet(name: *u8) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Single param: func square(x: i32): i32 { } ---- */

TEST_F(dt_statement_function, single_param) {
  const char *source = "func square(x: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(fn->arguments, 0);
  EXPECT_EQ(arg->super.kind, CUBEC_NODE_FUNCTION_ARGUMENT);
  EXPECT_NE(arg->identifier, nullptr);
  EXPECT_NE(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Multiple params: func sum(a: i32, b: i32, c: i32): i32 { } ---- */

TEST_F(dt_statement_function, multiple_params) {
  const char *source = "func sum(a: i32, b: i32, c: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic functions
 * ========================================================================== */

/* ---- Generic single param: func identity[T](x: T): T{ } ---- */

TEST_F(dt_statement_function, generic_single_param) {
  const char *source = "func identity[T](x: T): T{ }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(fn->generic_params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(fn->generic_params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Generic multiple params: func[T extends Numeric, N: u64](arr: [N]T): T{ } ---- */

TEST_F(dt_statement_function, generic_multiple_params) {
  const char *source = "func foo[T extends Numeric, N: u64](arr: [N]T): T{ }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(fn->generic_params), 2);

  /* First: T extends Numeric */
  cubec_generic_param_t param0 = (cubec_generic_param_t)vec_get(fn->generic_params, 0);
  EXPECT_NE(param0->constraints, nullptr);
  EXPECT_EQ(param0->value_type, nullptr);

  /* Second: N: u64 */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(fn->generic_params, 1);
  EXPECT_EQ(param1->constraints, nullptr);
  EXPECT_NE(param1->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Generic rest param: func[...Args](args: Args) { } ---- */

TEST_F(dt_statement_function, generic_rest_param) {
  const char *source = "func foo[...Args](args: Args) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(fn->generic_params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(fn->generic_params, 0);
  EXPECT_TRUE(param->is_rest);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Modifiers
 * ========================================================================== */

/* ---- Export function: export func create(): Vec[i32] { } ---- */

TEST_F(dt_statement_function, export_function) {
  const char *source = "export func create(): Vec[i32] { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_export);
  EXPECT_FALSE(fn->is_inline);
  EXPECT_FALSE(fn->is_extern);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Inline function: inline func helper(x: i32): i32 { } ---- */

TEST_F(dt_statement_function, inline_function) {
  const char *source = "inline func helper(x: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_FALSE(fn->is_export);
  EXPECT_TRUE(fn->is_inline);
  EXPECT_FALSE(fn->is_extern);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- No modifier: all bools false ---- */

TEST_F(dt_statement_function, no_modifier) {
  const char *source = "func foo() { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_FALSE(fn->is_export);
  EXPECT_FALSE(fn->is_inline);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Extern functions
 * ========================================================================== */

/* ---- Extern function: extern func read_file(path: *u8): []u8; ---- */

TEST_F(dt_statement_function, extern_function) {
  const char *source = "extern func read_file(path: *u8): []u8;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_extern);
  EXPECT_EQ(fn->body, nullptr);
  EXPECT_NE(fn->return_type, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Extern C-style variadic: extern func printf(fmt: *u8, ...): i32; ---- */

TEST_F(dt_statement_function, extern_c_variadic) {
  const char *source = "extern func printf(fmt: *u8, ...): i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_extern);
  EXPECT_TRUE(fn->is_c_variadic);
  EXPECT_EQ(fn->body, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Extern no params: extern func cleanup(); ---- */

TEST_F(dt_statement_function, extern_no_params) {
  const char *source = "extern func cleanup();";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_extern);
  EXPECT_EQ(fn->body, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 0);
  EXPECT_FALSE(fn->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Parameter types
 * ========================================================================== */

/* ---- Pointer type param ---- */

TEST_F(dt_statement_function, param_pointer_type) {
  const char *source = "func foo(p: *i32) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(fn->arguments, 0);
  EXPECT_NE(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Slice type param ---- */

TEST_F(dt_statement_function, param_slice_type) {
  const char *source = "func bar(s: []i32) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(fn->arguments, 0);
  EXPECT_NE(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Generic type param ---- */

TEST_F(dt_statement_function, param_generic_type) {
  const char *source = "func baz(v: Vec[i32]) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(fn->arguments, 0);
  EXPECT_NE(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- No type param ---- */

TEST_F(dt_statement_function, param_no_type) {
  const char *source = "func qux(x) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 1);

  cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(fn->arguments, 0);
  EXPECT_NE(arg->identifier, nullptr);
  EXPECT_EQ(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Function body
 * ========================================================================== */

/* ---- Empty body ---- */

TEST_F(dt_statement_function, empty_body) {
  const char *source = "func noop() { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->body, nullptr);
  EXPECT_EQ(fn->body->kind, CUBEC_NODE_STATEMENT_BLOCK);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Body with statements ---- */

TEST_F(dt_statement_function, body_with_statements) {
  const char *source = "func f() { var x = 42; x; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- No body semicolon (interface style) ---- */

TEST_F(dt_statement_function, no_body_semicolon) {
  const char *source = "func next(self: *Self): Item;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(fn->body, nullptr);
  EXPECT_NE(fn->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Error cases
 * ========================================================================== */

/* ---- Missing function name ---- */

TEST_F(dt_statement_function, missing_name) {
  const char *source = "func (a: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Missing open paren ---- */

TEST_F(dt_statement_function, missing_open_paren) {
  const char *source = "func add a: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Missing close paren ---- */

TEST_F(dt_statement_function, missing_close_paren) {
  const char *source = "func add(a: i32 -> i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Export + inline combined ---- */

TEST_F(dt_statement_function, export_inline_combined) {
  const char *source = "export inline func foo() { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_EQ(fn->is_export, true);
  EXPECT_EQ(fn->is_inline, true);
  EXPECT_EQ(fn->is_extern, false);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export + extern conflict ---- */

TEST_F(dt_statement_function, export_extern_conflict) {
  const char *source = "export extern func foo();";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Inline + extern conflict ---- */

TEST_F(dt_statement_function, inline_extern_conflict) {
  const char *source = "inline extern func foo();";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- C-style variadic in non-extern function ---- */

TEST_F(dt_statement_function, c_variadic_non_extern) {
  const char *source = "func foo(x: i32, ...): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

/* ---- Clone basic function ---- */

TEST_F(dt_statement_function, clone) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)cloned;
  EXPECT_NE(fn->name, nullptr);
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 2);
  EXPECT_NE(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone generic function ---- */

TEST_F(dt_statement_function, clone_generic) {
  const char *source = "func identity[T](x: T): T{ }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)cloned;
  EXPECT_NE(fn->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(fn->generic_params), 1);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_function, move) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)moved;
  EXPECT_NE(fn->name, nullptr);
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_NE(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone extern function ---- */

TEST_F(dt_statement_function, clone_extern) {
  const char *source = "extern func read_file(path: *u8): []u8;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)cloned;
  EXPECT_TRUE(fn->is_extern);
  EXPECT_EQ(fn->body, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

/* ---- Via read_statement dispatcher ---- */

TEST_F(dt_statement_function, via_read_statement) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_program_node ---- */

TEST_F(dt_statement_function, via_read_program) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_function, consume_all_tokens) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Builtin function
 * ========================================================================== */

/* ---- Builtin func: builtin func panic(msg: []u8): void; ---- */

TEST_F(dt_statement_function, builtin_func) {
  const char *source = "builtin func panic(msg: []u8): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_builtin);
  EXPECT_FALSE(fn->is_export);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_inline);
  EXPECT_EQ(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export builtin func (orthogonal) ---- */

TEST_F(dt_statement_function, export_builtin_func) {
  const char *source = "export builtin func foo(): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_export);
  EXPECT_TRUE(fn->is_builtin);
  EXPECT_FALSE(fn->is_extern);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: builtin and extern are mutually exclusive ---- */

TEST_F(dt_statement_function, builtin_extern_mutual_exclusion) {
  const char *source = "builtin extern func foo(): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Comptime functions
 * ========================================================================== */

/* ---- Comptime func: comptime func fib(n: u64): u64 { } ---- */

TEST_F(dt_statement_function, comptime_func) {
  const char *source = "comptime func fib(n: u64): u64 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_comptime);
  EXPECT_FALSE(fn->is_export);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_builtin);
  EXPECT_FALSE(fn->is_inline);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: comptime func without body ---- */

TEST_F(dt_statement_function, comptime_func_without_body_is_error) {
  const char *source = "comptime func foo(): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Export comptime func (orthogonal combination) ---- */

TEST_F(dt_statement_function, export_comptime_func) {
  const char *source = "export comptime func MAX(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_export);
  EXPECT_TRUE(fn->is_comptime);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_builtin);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Inline comptime func (orthogonal combination) ---- */

TEST_F(dt_statement_function, inline_comptime_func) {
  const char *source = "inline comptime func fib(n: u64): u64 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_function_t fn = (cubec_statement_function_t)node;
  EXPECT_TRUE(fn->is_inline);
  EXPECT_TRUE(fn->is_comptime);
  EXPECT_FALSE(fn->is_extern);
  EXPECT_FALSE(fn->is_builtin);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: builtin and comptime are mutually exclusive ---- */

TEST_F(dt_statement_function, builtin_comptime_func_mutual_exclusion) {
  const char *source = "builtin comptime func foo(): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* ---- Error: extern and comptime are mutually exclusive ---- */

TEST_F(dt_statement_function, extern_comptime_func_mutual_exclusion) {
  const char *source = "extern comptime func foo(): void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_function, write_basic_function) {
  const char *source = "func add(a: i32, b: i32): i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_statement(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, "func add(a: i32, b: i32): i32 {\n}\n");
  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
