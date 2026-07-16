#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

struct compile_result {
  checker_t ctx;
  node_t prog;
  vec_t tokens;
};

/**
 * Compile a Cubec source string through the full pipeline
 * (lex -> parse -> checker). Caller must call compile_result_cleanup.
 */
static struct compile_result compile_source(allocator_t allocator,
                                            const char *source) {
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(allocator, tokens, &pos, "test.cubec");
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);
  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_e2e : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== Batch 1: Arithmetic and comparison ===== */

TEST_F(dt_e2e, arithmetic_i32) {
  const char *src =
    "test \"i32_add\" { assert(1 + 2 == 3); }\n"
    "test \"i32_sub\" { assert(10 - 3 == 7); }\n"
    "test \"i32_mul\" { assert(6 * 7 == 42); }\n"
    "test \"i32_div\" { assert(10 / 3 == 3); }\n"
    "test \"i32_mod\" { assert(10 % 3 == 1); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 5);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, arithmetic_i32_neg) {
  const char *src =
    "test \"neg\" { assert(-5 + 10 == 5); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, comparison) {
  const char *src =
    "test \"lt\" { assert(1 < 2); }\n"
    "test \"gt\" { assert(3 > 2); }\n"
    "test \"le\" { assert(2 <= 2); }\n"
    "test \"ge\" { assert(2 >= 2); }\n"
    "test \"eq\" { assert(42 == 42); }\n"
    "test \"ne\" { assert(1 != 2); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 6);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, precedence_mul_add) {
  const char *src =
    "test \"mul_before_add\" { assert(2 + 3 * 4 == 14); }\n"
    "test \"paren_override\" { assert((2 + 3) * 4 == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 2: Logical and bitwise ===== */

TEST_F(dt_e2e, logical) {
  const char *src =
    "test \"and_true\" { assert(true && true); }\n"
    "test \"and_false\" { assert(!(true && false)); }\n"
    "test \"or_true\" { assert(false || true); }\n"
    "test \"or_false\" { assert(!(false || false)); }\n"
    "test \"not\" { assert(!false); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 5);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, bitwise) {
  const char *src =
    "test \"bitand\" { assert((12 & 10) == 8); }\n"
    "test \"bitor\" { assert((12 | 10) == 14); }\n"
    "test \"bitxor\" { assert((12 ^ 10) == 6); }\n"
    "test \"shift_left\" { assert((1 << 3) == 8); }\n"
    "test \"shift_right\" { assert((8 >> 2) == 2); }\n"
    "test \"bitnot\" { assert(~0 != 0); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 6);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 3: Variables and assignment ===== */

TEST_F(dt_e2e, variable_decl) {
  const char *src =
    "test \"var_typed\" { var x: i32 = 10; assert(x == 10); }\n"
    "test \"var_inferred\" { var y = 20; assert(y == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, variable_assign) {
  const char *src =
    "test \"assign\" { var x = 5; x = 20; assert(x == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, scope_shadow) {
  const char *src =
    "test \"shadow\" { var x = 1; { var x = 2; assert(x == 2); } assert(x == 1); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 4: Control flow ===== */

TEST_F(dt_e2e, if_else) {
  const char *src =
    "test \"if_true\" { var x = 0; if (true) { x = 1; } assert(x == 1); }\n"
    "test \"if_else\" { var x = 0; if (false) { x = 1; } else { x = 2; } assert(x == 2); }\n"
    "test \"if_else_if\" { var x = 0; if (false) { x = 1; } else if (true) { x = 2; } else { x = 3; } assert(x == 2); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 3);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, while_loop) {
  const char *src =
    "test \"while\" { var i = 0; while (i < 3) { i = i + 1; } assert(i == 3); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, for_loop) {
  const char *src =
    "test \"for\" { var sum = 0; for (var i = 0; i < 5; i = i + 1) { sum = sum + i; } assert(sum == 10); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, break_continue) {
  const char *src =
    "test \"break\" { var i = 0; while (true) { i = i + 1; break; } assert(i == 1); }\n"
    "test \"continue\" { var sum = 0; for (var i = 0; i < 5; i = i + 1) { if (i == 2) continue; sum = sum + 1; } assert(sum == 4); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 5: Functions ===== */

TEST_F(dt_e2e, function_call) {
  const char *src =
    "func add(a: i32, b: i32): i32 { return a + b; }\n"
    "test \"call_add\" { assert(add(3, 4) == 7); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, recursion) {
  const char *src =
    "func fib(n: i32): i32 { if (n <= 1) { return n; } return fib(n - 1) + fib(n - 2); }\n"
    "test \"fib10\" { assert(fib(10) == 55); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 6: Struct ===== */

TEST_F(dt_e2e, struct_init_and_access) {
  const char *src =
    "struct Point { x: i32; y: i32; }\n"
    "test \"struct_init\" { var p = .Point { .x = 1, .y = 2 }; assert(p.x == 1); assert(p.y == 2); }\n"
    "test \"struct_assign\" { var p = .Point { .x = 0, .y = 0 }; p.x = 99; assert(p.x == 99); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 7: Type operations ===== */

TEST_F(dt_e2e, typeof_expr) {
  const char *src =
    "test \"typeof_i32\" { assert(typeof(42) == typeof(1)); }\n"
    "test \"typeof_i64\" { assert(typeof(1i64) == typeof(1i64)); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, sizeof_alignof) {
  const char *src =
    "test \"sizeof_i32\" { assert(sizeof(i32) == 4); }\n"
    "test \"sizeof_i64\" { assert(sizeof(i64) == 8); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, ternary) {
  const char *src =
    "test \"ternary_true\" { assert(true ? 10 : 20 == 10); }\n"
    "test \"ternary_false\" { assert(false ? 10 : 20 == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Failure isolation ===== */

TEST_F(dt_e2e, failure_isolation) {
  const char *src =
    "test \"fail1\" { assert(false); }\n"
    "test \"pass1\" { assert(true); }\n"
    "test \"fail2\" { assert(1 == 2); }\n"
    "test \"pass2\" { assert(2 + 2 == 4); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 4);
  EXPECT_EQ(r.ctx->test_fail_count, 2);
  compile_result_cleanup(&r, allocator);
}
