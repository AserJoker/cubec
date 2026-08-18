#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/str_type.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_member.h"
#include "cubec/expression_call.h"
#include "cubec/expression_subscript.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_block.h"
#include "cubec/statement_empty.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ------------------------------------------------------------------ *
 *  it_run_statement — end-to-end tests via lexer→parser→run_program
 *
 *  Tests build source strings, tokenize them with resolve_token_list,
 *  parse with read_program_node, and execute with run_program.
 *  Variables are pre-bound in the vm scope to exercise member/call/
 *  subscript paths that need runtime objects.
 * ------------------------------------------------------------------ */

class it_run_statement : public CubecTest {
protected:
  vm_t vm() { return ctx->vm; }

  void free_node(node_t &node) {
    if (node) allocator_free(ctx->allocator, &node);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(ctx->allocator, &tokens);
  }

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm());
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
  }

  /* Parse source code into a program node via lexer→parser */
  node_t _parse(const char *source) {
    vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  /* Run a program node */
  value_t _run(node_t node, bool shadow = false) {
    return run_program(ctx, node, shadow);
  }

  /* Parse + run a source string in one step */
  value_t _run_source(const char *source, bool shadow = false) {
    node_t node = _parse(source);
    value_t v = _run(node, shadow);
    free_node(node);
    return v;
  }

  size_t _error_count() {
    return diagnostic_list_get_error_count(ctx->diagnostics);
  }

  void _clear_diagnostics() {
    diagnostic_list_clear(ctx->diagnostics);
  }
};

/* ==================================================================
 *  run_statement dispatcher
 * ================================================================== */

TEST_F(it_run_statement, null_node_returns_void) {
  value_t v = run_statement(ctx, NULL, false);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Empty program / empty statement
 * ================================================================== */

TEST_F(it_run_statement, empty_program_returns_void) {
  value_t v = _run_source("");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_statement, empty_statement_returns_void) {
  value_t v = _run_source(";");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Expression statement — assignment returns void (valid)
 * ================================================================== */

TEST_F(it_run_statement, expression_statement_assignment_void_ok) {
  value_t i32_val = create_i32_value(vm(), 0);
  _bind("x", i32_val);

  value_t v = _run_source("x = 42;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  /* verify x was updated */
  scope_t scope = vm_get_current_scope(vm());
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 42);
}

TEST_F(it_run_statement, expression_statement_discard_wildcard) {
  /* _ = <expr> discards the right-hand value */
  value_t v = _run_source("_ = 42;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Expression statement — non-void result is error
 * ================================================================== */

TEST_F(it_run_statement, expression_statement_non_void_returns_error) {
  /* bare i32 expression is non-void → error in script mode */
  value_t v = _run_source("42;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement, expression_statement_non_void_shadow_writes_diagnostic) {
  size_t before = _error_count();
  value_t v = _run_source("42;", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_GT(diagnostic_list_get_error_count(ctx->diagnostics), before);
  _clear_diagnostics();
}

/* ==================================================================
 *  Expression statement — shadow error absorbed
 * ================================================================== */

TEST_F(it_run_statement, expression_statement_shadow_error_absorbed) {
  /* type mismatch: i32 + str → error in shadow mode → diagnostic + void */
  size_t before = _error_count();
  value_t v = _run_source("1 + \"bad\";", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_GT(diagnostic_list_get_error_count(ctx->diagnostics), before);
  _clear_diagnostics();
}

TEST_F(it_run_statement, expression_statement_script_error_propagated) {
  value_t v = _run_source("1 + \"bad\";");
  EXPECT_TRUE(value_is_error(v));
}

/* ==================================================================
 *  Block statement
 * ================================================================== */

TEST_F(it_run_statement, block_empty_returns_void) {
  value_t v = _run_source("{}");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_statement, block_with_void_statements) {
  value_t x = create_i32_value(vm(), 0);
  value_t y = create_i32_value(vm(), 0);
  _bind("x", x);
  _bind("y", y);

  value_t v = _run_source("{ x = 1; y = 2; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm());
  name_t nx = scope_lookup(scope, "x");
  ASSERT_NE(nx, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(nx->ref), 1);
  name_t ny = scope_lookup(scope, "y");
  ASSERT_NE(ny, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(ny->ref), 2);
}

TEST_F(it_run_statement, block_error_propagated) {
  /* block with non-void expression statement → error in script mode */
  value_t v = _run_source("{ 42; }");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement, block_shadow_error_continues) {
  size_t before = _error_count();

  value_t z = create_i32_value(vm(), 0);
  _bind("z", z);

  /* first statement errors (non-void), second is valid; shadow mode
   * should write diagnostic and continue, returning void.
   * Note: shadow mode does not perform actual writes, so z stays 0 */
  value_t v = _run_source("{ 42; z = 99; }", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_GT(diagnostic_list_get_error_count(ctx->diagnostics), before);

  _clear_diagnostics();
}

TEST_F(it_run_statement, block_creates_scope) {
  /* variables defined inside block should not leak to outer scope */
  value_t x = create_i32_value(vm(), 1);
  _bind("x", x);

  value_t v = _run_source("{ x = 2; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  /* x should be updated (assignment in inner scope affects same name) */
  scope_t scope = vm_get_current_scope(vm());
  name_t nx = scope_lookup(scope, "x");
  ASSERT_NE(nx, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(nx->ref), 2);
}

/* ==================================================================
 *  Multiple statements in program
 * ================================================================== */

TEST_F(it_run_statement, program_multiple_statements) {
  value_t x = create_i32_value(vm(), 0);
  _bind("x", x);

  value_t v = _run_source("x = 1; x = 2; x = 3;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm());
  name_t nx = scope_lookup(scope, "x");
  ASSERT_NE(nx, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(nx->ref), 3);
}

/* ==================================================================
 *  Nested blocks
 * ================================================================== */

TEST_F(it_run_statement, nested_blocks) {
  value_t x = create_i32_value(vm(), 0);
  _bind("x", x);

  value_t v = _run_source("{ x = 1; { x = 2; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm());
  name_t nx = scope_lookup(scope, "x");
  ASSERT_NE(nx, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(nx->ref), 2);
}

/* ==================================================================
 *  Compound assignment
 * ================================================================== */

TEST_F(it_run_statement, compound_assignment_in_statement) {
  value_t y = create_i32_value(vm(), 10);
  _bind("y", y);

  value_t v = _run_source("y += 5;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm());
  name_t ny = scope_lookup(scope, "y");
  ASSERT_NE(ny, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(ny->ref), 15);
}

/* ==================================================================
 *  Assignment with safe_cast in statement context
 * ================================================================== */

TEST_F(it_run_statement, assign_i32_literal_to_i8_rejected) {
  /* x: i8 = 42; — i32→i8 narrowing, safe_cast rejects */
  value_t x = create_i8_value(vm(), 0);
  _bind("x", x);

  value_t v = _run_source("x = 42;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement, assign_i64_to_i32_rejected) {
  /* y: i32 = big; — i64→i32 narrowing, safe_cast rejects */
  value_t y = create_i32_value(vm(), 0);
  _bind("y", y);
  value_t big = create_i64_value(vm(), 0x1FFFFFFFF);
  _bind("big", big);

  value_t v = _run_source("y = big;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement, assign_incompatible_type_statement_error) {
  /* x: i32 = true; — safe_cast bool→i32 fails, error propagates */
  value_t x = create_i32_value(vm(), 0);
  _bind("x", x);

  value_t v = _run_source("x = true;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement, compound_assign_same_type) {
  /* y: i8 += 3i8; — same type compound assignment, no widening */
  value_t y = create_i8_value(vm(), 5);
  _bind("y", y);

  value_t v = _run_source("y += 3i8;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm());
  name_t ny = scope_lookup(scope, "y");
  ASSERT_NE(ny, nullptr);
  EXPECT_EQ(*(int8_t *)value_get_data(ny->ref), 8);
}

TEST_F(it_run_statement, compound_assign_widening_then_narrowing_rejected) {
  /* y: i8 += 10; — i8(5) + i32(10) promotes to i32(15), then
   * safe_cast i32→i8 is narrowing → rejected */
  value_t y = create_i8_value(vm(), 5);
  _bind("y", y);

  value_t v = _run_source("y += 10;");
  EXPECT_TRUE(value_is_error(v));
}
