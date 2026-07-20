#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

/* Common builtin declarations prepended to every test source */
#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"

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

  /* If parsing failed, fail the test immediately */
  if (g_error) {
    std::string err_msg(g_error->message);
    error_clear();
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        ("Parsing failed: " + err_msg).c_str(),
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  if (r->ctx) checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_e2e : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== Batch 1: Arithmetic and comparison ===== */

TEST_F(dt_e2e, arithmetic_i32) {
  const char *src = BUILTIN_ASSERT
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
  const char *src = BUILTIN_ASSERT
    "test \"neg\" { assert(-5 + 10 == 5); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, comparison) {
  const char *src = BUILTIN_ASSERT
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
  const char *src = BUILTIN_ASSERT
    "test \"mul_before_add\" { assert(2 + 3 * 4 == 14); }\n"
    "test \"paren_override\" { assert((2 + 3) * 4 == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 2: Logical and bitwise ===== */

TEST_F(dt_e2e, logical) {
  const char *src = BUILTIN_ASSERT
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
  const char *src = BUILTIN_ASSERT
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
  const char *src = BUILTIN_ASSERT
    "test \"var_typed\" { var x: i32 = 10; assert(x == 10); }\n"
    "test \"var_inferred\" { var y = 20; assert(y == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, variable_assign) {
  const char *src = BUILTIN_ASSERT
    "test \"assign\" { var x = 5; x = 20; assert(x == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, scope_shadow) {
  const char *src = BUILTIN_ASSERT
    "test \"shadow\" { var x = 1; { var x = 2; assert(x == 2); } assert(x == 1); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 4: Control flow ===== */

TEST_F(dt_e2e, if_else) {
  const char *src = BUILTIN_ASSERT
    "test \"if_true\" { var x = 0; if (true) { x = 1; } assert(x == 1); }\n"
    "test \"if_else\" { var x = 0; if (false) { x = 1; } else { x = 2; } assert(x == 2); }\n"
    "test \"if_else_if\" { var x = 0; if (false) { x = 1; } else if (true) { x = 2; } else { x = 3; } assert(x == 2); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 3);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, while_loop) {
  const char *src = BUILTIN_ASSERT
    "test \"while\" { var i = 0; while (i < 3) { i = i + 1; } assert(i == 3); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, for_loop) {
  const char *src = BUILTIN_ASSERT
    "test \"for\" { var sum = 0; for (var i = 0; i < 5; i = i + 1) { sum = sum + i; } assert(sum == 10); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, break_continue) {
  const char *src = BUILTIN_ASSERT
    "test \"break\" { var i = 0; while (true) { i = i + 1; break; } assert(i == 1); }\n"
    "test \"continue\" { var sum = 0; for (var i = 0; i < 5; i = i + 1) { if (i == 2) continue; sum = sum + 1; } assert(sum == 4); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 5: Functions ===== */

TEST_F(dt_e2e, function_call) {
  const char *src = BUILTIN_ASSERT
    "func add(a: i32, b: i32): i32 { return a + b; }\n"
    "test \"call_add\" { assert(add(3, 4) == 7); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, recursion) {
  const char *src = BUILTIN_ASSERT
    "func fib(n: i32): i32 { if (n <= 1) { return n; } return fib(n - 1) + fib(n - 2); }\n"
    "test \"fib10\" { assert(fib(10) == 55); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 6: Struct ===== */

TEST_F(dt_e2e, struct_init_and_access) {
  const char *src = BUILTIN_ASSERT
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
  const char *src = BUILTIN_ASSERT
    "test \"typeof_i32\" { assert(typeof(42) == typeof(1)); }\n"
    "test \"typeof_i64\" { assert(typeof(1i64) == typeof(1i64)); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, sizeof_alignof) {
  const char *src = BUILTIN_ASSERT
    "test \"sizeof_i32\" { assert(sizeof(i32) == 4); }\n"
    "test \"sizeof_i64\" { assert(sizeof(i64) == 8); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, ternary) {
  const char *src = BUILTIN_ASSERT
    "test \"ternary_true\" { assert((true ? 10 : 20) == 10); }\n"
    "test \"ternary_false\" { assert((false ? 10 : 20) == 20); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 2);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Failure isolation ===== */

TEST_F(dt_e2e, failure_isolation) {
  const char *src = BUILTIN_ASSERT
    "test \"fail1\" { assert(false); }\n"
    "test \"pass1\" { assert(true); }\n"
    "test \"fail2\" { assert(1 == 2); }\n"
    "test \"pass2\" { assert(2 + 2 == 4); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 4);
  EXPECT_EQ(r.ctx->test_fail_count, 2);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 8: Pointers — address-of, dereference ===== */

TEST_F(dt_e2e, addr_and_deref) {
  const char *src = BUILTIN_ASSERT
    "test \"addr_deref\" {\n"
    "  var x: i32 = 42;\n"
    "  var p: *i32 = x.&;\n"
    "  assert(p.* == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, deref_assign) {
  /* deref-assign through pointer modifies the original variable */
  const char *src = BUILTIN_ASSERT
    "test \"deref_assign\" {\n"
    "  var x: i32 = 10;\n"
    "  var p: *i32 = x.&;\n"
    "  p.* = 99;\n"
    "  assert(x == 99);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, pointer_field_access) {
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"ptr_field\" {\n"
    "  var p = .Point { .x = 1, .y = 2 };\n"
    "  var pp: *Point = p.&;\n"
    "  assert(pp.x == 1);\n"
    "  assert(pp.y == 2);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, pointer_deref_field) {
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"deref_then_field\" {\n"
    "  var p = .Point { .x = 3, .y = 4 };\n"
    "  var pp: *Point = p.&;\n"
    "  assert(pp.*.x == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 9: Pointer restrictions ===== */

TEST_F(dt_e2e, pointer_no_arithmetic) {
  const char *src = BUILTIN_ASSERT
    "test \"ptr_no_add\" {\n"
    "  var x: i32 = 0;\n"
    "  var p: *i32 = x.&;\n"
    "  var q = p + 1;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, pointer_no_subtract) {
  const char *src = BUILTIN_ASSERT
    "test \"ptr_no_sub\" {\n"
    "  var x: i32 = 0;\n"
    "  var p: *i32 = x.&;\n"
    "  var q = p - 1;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, pointer_no_index) {
  const char *src = BUILTIN_ASSERT
    "test \"ptr_no_index\" {\n"
    "  var x: i32 = 0;\n"
    "  var p: *i32 = x.&;\n"
    "  var v = p[0];\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 10: Member call ===== */

TEST_F(dt_e2e, object_method_call) {
  const char *src = BUILTIN_ASSERT
    "struct Counter {\n"
    "  value: i32;\n"
    "  func get(self: *Counter): i32 { return self.value; }\n"
    "}\n"
    "test \"method_call\" {\n"
    "  var c = .Counter { .value = 42 };\n"
    "  assert(c.get() == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, pointer_method_call) {
  const char *src = BUILTIN_ASSERT
    "struct Counter {\n"
    "  value: i32;\n"
    "  func get(self: *Counter): i32 { return self.value; }\n"
    "}\n"
    "test \"ptr_method_call\" {\n"
    "  var c = .Counter { .value = 7 };\n"
    "  var pc: *Counter = c.&;\n"
    "  assert(pc.get() == 7);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 5: Pointer reference semantics ===== */

TEST_F(dt_e2e, pointer_write_reflects_to_var) {
  /* pa.x = 1 modifies a.x because pa points to a's alloc slot */
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"ptr_write_reflects\" {\n"
    "  var a = .Point { .x = 1, .y = 2 };\n"
    "  var pa = a.&;\n"
    "  pa.x = 99;\n"
    "  assert(a.x == 99);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, deref_write_reflects_to_var) {
  /* p.* = 99 modifies x because p points to x's alloc slot */
  const char *src = BUILTIN_ASSERT
    "test \"deref_write_reflects\" {\n"
    "  var x: i32 = 10;\n"
    "  var p: *i32 = x.&;\n"
    "  p.* = 99;\n"
    "  assert(x == 99);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, deref_member_write_reflects_to_var) {
  /* pa.*.x = 99 modifies a.x — same as pa.x = 99 */
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"deref_member_write_reflects\" {\n"
    "  var a = .Point { .x = 1, .y = 2 };\n"
    "  var pa = a.&;\n"
    "  pa.*.x = 99;\n"
    "  assert(a.x == 99);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Batch 11: const/volatile semantics ===== */

TEST_F(dt_e2e, const_var_assign_error) {
  /* Assigning to a const variable should produce an error */
  const char *src = BUILTIN_ASSERT
    "test \"const_assign\" {\n"
    "  var x: const i32 = 10;\n"
    "  x = 20;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_var_read_ok) {
  /* Reading a const variable is fine */
  const char *src = BUILTIN_ASSERT
    "test \"const_read\" {\n"
    "  var x: const i32 = 42;\n"
    "  assert(x == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_field_assign_error) {
  /* Assigning to a field of a const struct should produce an error */
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"const_field_assign\" {\n"
    "  var p: const Point = .Point { .x = 1, .y = 2 };\n"
    "  p.x = 99;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, ptr_to_const_deref_assign_error) {
  /* *p where p: *const i32 should not allow assignment */
  const char *src = BUILTIN_ASSERT
    "test \"ptr_to_const_deref_assign\" {\n"
    "  var x: i32 = 10;\n"
    "  var p: *const i32 = x.&;\n"
    "  p.* = 99;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_ptr_reassign_error) {
  /* p where p: const *i32 should not allow reassignment */
  const char *src = BUILTIN_ASSERT
    "test \"const_ptr_reassign\" {\n"
    "  var x: i32 = 10;\n"
    "  var y: i32 = 20;\n"
    "  var p: const *i32 = x.&;\n"
    "  p = y.&;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_volatile_type) {
  /* const volatile i32 should compile without error */
  const char *src = BUILTIN_ASSERT
    "test \"const_volatile\" {\n"
    "  var x: const volatile i32 = 42;\n"
    "  assert(x == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_member_access) {
  /* Reading a field of a const struct is fine */
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"const_member_read\" {\n"
    "  var p: const Point = .Point { .x = 1, .y = 2 };\n"
    "  assert(p.x == 1);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, const_ptr_deref_read_ok) {
  /* Reading through *const T pointer is fine */
  const char *src = BUILTIN_ASSERT
    "test \"const_ptr_deref_read\" {\n"
    "  var x: i32 = 10;\n"
    "  var p: *const i32 = x.&;\n"
    "  assert(p.* == 10);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== anonymous initialize_list tests ===== */

TEST_F(dt_e2e, anon_init_list_named_fields) {
  /* .{.x=1, .y=2} infers anonymous struct */
  const char *src = BUILTIN_ASSERT
    "test \"anon_struct\" {\n"
    "  var p = .{ .x = 1, .y = 2 };\n"
    "  assert(p.x == 1);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, anon_init_list_positional_tuple) {
  /* .{1, 2} infers tuple <i32, i32> — compile test only */
  const char *src = BUILTIN_ASSERT
    "test \"anon_tuple\" {\n"
    "  var t = .{ 1, 2 };\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, anon_init_list_empty_struct) {
  /* .{} infers empty struct */
  const char *src = BUILTIN_ASSERT
    "test \"empty_struct\" {\n"
    "  var e = .{};\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, tuple_to_array_implicit) {
  /* tuple <i32, i32> implicitly converts to [2]i32 */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_to_array\" {\n"
    "  var a: [2]i32 = .{ 1, 2 };\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, anon_struct_to_named_struct) {
  /* anonymous struct implicitly converts to named struct with matching fields */
  const char *src = BUILTIN_ASSERT
    "struct Point { x: i32; y: i32; }\n"
    "test \"anon_to_named\" {\n"
    "  var p: Point = .{ .x = 1, .y = 2 };\n"
    "  assert(p.x == 1);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== opaque type tests ===== */

TEST_F(dt_e2e, opaque_from_pointer) {
  /* any pointer implicitly converts to opaque */
  const char *src = BUILTIN_ASSERT
    "test \"opaque_from_ptr\" {\n"
    "  var x: i32 = 10;\n"
    "  var p: opaque = x.&;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, opaque_no_implicit_convert_out) {
  /* opaque cannot implicitly convert to any type */
  const char *src = BUILTIN_ASSERT
    "test \"opaque_no_convert\" {\n"
    "  var p: opaque = .{};\n"
    "  var x: i32 = p;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== local struct/union methods ===== */

TEST_F(dt_e2e, local_struct_method_call) {
  const char *src = BUILTIN_ASSERT
    "test \"local_struct_method\" {\n"
    "  struct Counter {\n"
    "    value: i32;\n"
    "    func get(self: *Counter): i32 { return self.value; }\n"
    "  }\n"
    "  var c = .Counter { .value = 42 };\n"
    "  assert(c.get() == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, local_struct_using_dispose) {
  const char *src = BUILTIN_ASSERT
    "test \"local_using_dispose\" {\n"
    "  struct Item {\n"
    "    val: i32;\n"
    "    func __dispose__(self: *Item): void { assert(false); }\n"
    "  }\n"
    "  using a:Item = .Item { .val = 1 };\n"
    "}\n";
  auto r = compile_source(allocator, src);
  /* __dispose__ calls assert(false) → test should fail, proving dispose ran */
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_e2e, local_union_method) {
  const char *src = BUILTIN_ASSERT
    "test \"local_union_method\" {\n"
    "  union Val {\n"
    "    i_val: i32;\n"
    "    f_val: f64;\n"
    "    func as_int(self: *Val): i32 { return self.i_val; }\n"
    "  }\n"
    "  var v = .Val { .i_val = 7 };\n"
    "  assert(v.as_int() == 7);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}
