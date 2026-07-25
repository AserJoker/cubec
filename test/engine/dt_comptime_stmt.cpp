#include "engine/context.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"

struct compile_result {
  context_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_source(context_t ctx,
                                            const char *source) {
  allocator_t allocator = ctx->allocator;
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(ctx, tokens, &pos, "test.cubec");

  /* If parsing failed, fail the test immediately */
  if (!prog || !tokens) {
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        "Parsing failed",
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  source_cache_load(ctx->sources, "test.cubec", source, false);

  context_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

/* ===== test fixture ===== */

class dt_comptime_stmt : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== comptime var ===== */

TEST_F(dt_comptime_stmt, comptime_var) {
  const char *src =
    "comptime var x = 1 + 2;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_bool) {
  const char *src =
    "comptime var flag = true;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_string) {
  const char *src =
    "comptime var name = \"hello\";\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_undefined_error) {
  /* Global comptime var cannot be initialized with undefined */
  const char *src =
    "comptime var x: i32 = undefined;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_no_init_error) {
  /* Global comptime var must have an initializer — enforced at parse time */
  const char *src =
    "comptime var x: i32;\n";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", src);
  size_t pos = 0;
  node_t prog = read_program_node(ctx, tokens, &pos, "test.cubec");
  /* Parse-time error: prog should be null */
  EXPECT_EQ(prog, nullptr);
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_comptime_stmt, comptime_var_implicit_const) {
  /* Global comptime var is implicitly const — type auto-wrapped to const */
  const char *src =
    "comptime var x = 42;\n"
    "comptime var y: i32 = 10;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  /* Verify the types are const */
  struct symbol *x_sym = scope_lookup_local(r.ctx->global_scope, "x");
  ASSERT_NE(x_sym, nullptr);
  EXPECT_TRUE(semantic_type_is_const(x_sym->variable.type));
  struct symbol *y_sym = scope_lookup_local(r.ctx->global_scope, "y");
  ASSERT_NE(y_sym, nullptr);
  EXPECT_TRUE(semantic_type_is_const(y_sym->variable.type));
  compile_result_cleanup(&r, allocator);
}

/* ===== extends expression ===== */

TEST_F(dt_comptime_stmt, extends_comptime_eval) {
  /* extends expression should be evaluable at comptime and return bool */
  const char *src =
    "comptime var is_int = i32 extends i32;\n"
    "comptime var not_int = i32 extends f64;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, extends_in_comptime_if) {
  /* extends can be used in comptime if condition */
  const char *src =
    "comptime var cond = true;\n"
    "comptime if(cond) {\n"
    "  comptime var x = 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== comptime if ===== */

TEST_F(dt_comptime_stmt, comptime_if_true_branch_taken) {
  /* comptime if(true) — then branch taken, else branch NOT checked */
  const char *src =
    "comptime if(true) {\n"
    "  comptime var x = 1;\n"
    "} else {\n"
    "  comptime var y = nonexistent_type;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  /* x should exist (taken branch), y should NOT exist */
  struct symbol *x_sym = scope_lookup_local(r.ctx->global_scope, "x");
  EXPECT_NE(x_sym, nullptr);
  struct symbol *y_sym = scope_lookup_local(r.ctx->global_scope, "y");
  EXPECT_EQ(y_sym, nullptr);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_if_false_branch_taken) {
  /* comptime if(false) — else branch taken, then branch NOT checked */
  const char *src =
    "comptime if(false) {\n"
    "  comptime var y = nonexistent_type;\n"
    "} else {\n"
    "  comptime var x = 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  struct symbol *x_sym = scope_lookup_local(r.ctx->global_scope, "x");
  EXPECT_NE(x_sym, nullptr);
  struct symbol *y_sym = scope_lookup_local(r.ctx->global_scope, "y");
  EXPECT_EQ(y_sym, nullptr);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_if_non_bool_error) {
  /* comptime if with non-bool condition should error */
  const char *src =
    "comptime var x = 42;\n"
    "comptime if(x) {\n"
    "  comptime var y = 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_if_no_else) {
  /* comptime if(true) without else — should work */
  const char *src =
    "comptime if(true) {\n"
    "  comptime var x = 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  struct symbol *x_sym = scope_lookup_local(r.ctx->global_scope, "x");
  EXPECT_NE(x_sym, nullptr);
  compile_result_cleanup(&r, allocator);
}

/* ===== local comptime var ===== */

TEST_F(dt_comptime_stmt, local_comptime_var_basic) {
  /* Local comptime var should be evaluated at compile time */
  const char *src = BUILTIN_ASSERT
    "test \"local_comptime_var\" {\n"
    "  comptime var x = 1 + 2;\n"
    "  assert(x == 3);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, local_comptime_var_implicit_const) {
  /* Local comptime var is implicitly const — cannot be reassigned */
  const char *src = BUILTIN_ASSERT
    "test \"local_comptime_const\" {\n"
    "  comptime var x = 42;\n"
    "  x = 99;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, local_comptime_var_no_init_error) {
  /* Local comptime var must have an initializer — enforced at parse time */
  const char *src =
    "comptime var x: i32;\n";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", src);
  size_t pos = 0;
  node_t prog = read_program_node(ctx, tokens, &pos, "test.cubec");
  EXPECT_EQ(prog, nullptr);
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_comptime_stmt, local_comptime_var_undefined_error) {
  /* Local comptime var cannot be initialized with undefined */
  const char *src = BUILTIN_ASSERT
    "test \"local_comptime_undefined\" {\n"
    "  comptime var x: i32 = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== local comptime func ===== */

TEST_F(dt_comptime_stmt, local_comptime_func_basic) {
  /* Local comptime func should be bound to comptime env */
  const char *src = BUILTIN_ASSERT
    "test \"local_comptime_func\" {\n"
    "  comptime func double(x: i32): i32 { return x + x; }\n"
    "  comptime var y = double(21);\n"
    "  assert(y == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
