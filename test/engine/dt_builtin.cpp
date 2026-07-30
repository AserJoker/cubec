#include "engine/context.h"
#include "engine/builtin.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
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

class dt_builtin : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Builtin table API ===== */

TEST_F(dt_builtin, table_create_dispose) {
  context_t ctx = context_create(allocator);
  ASSERT_NE(ctx->builtin_table, nullptr);
  context_dispose(ctx);
}

TEST_F(dt_builtin, table_lookup_assert) {
  context_t ctx = context_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "assert");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  context_dispose(ctx);
}

TEST_F(dt_builtin, table_lookup_unknown) {
  context_t ctx = context_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "nonexistent");
  EXPECT_EQ(be, nullptr);
  context_dispose(ctx);
}

/* ===== Builtin declaration validation ===== */

TEST_F(dt_builtin, assert_declared_correctly) {
  const char *src = "builtin func assert(condition: bool): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);

  /* Symbol should be marked as builtin */
  struct symbol *sym = scope_lookup(r.ctx->global_scope, "assert");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, unknown_builtin_declaration) {
  const char *src = "builtin func notexist(x: i32): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_signature_mismatch) {
  /* assert expects (bool): void, not (i32): void */
  const char *src = "builtin func assert(condition: i32): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_kind_mismatch) {
  /* 'assert' is a builtin func, declaring as var should error */
  const char *src = "builtin var assert: bool;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_assert_e2e) {
  const char *src =
    "builtin func assert(condition: bool): void;\n"
    "test \"builtin_assert\" { assert(true); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, non_builtin_func_not_marked) {
  const char *src =
    "builtin func assert(condition: bool): void;\n"
    "func foo(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);

  /* foo is not a builtin */
  struct symbol *foo_sym = scope_lookup(r.ctx->global_scope, "foo");
  ASSERT_NE(foo_sym, nullptr);
  EXPECT_FALSE(foo_sym->is_builtin);

  /* assert is a builtin */
  struct symbol *assert_sym = scope_lookup(r.ctx->global_scope, "assert");
  ASSERT_NE(assert_sym, nullptr);
  EXPECT_TRUE(assert_sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

/* ===== Tuple type (TYPE_TUPLE, not builtin) ===== */

TEST_F(dt_builtin, table_lookup_tuple) {
  /* Tuple is no longer a builtin — lookup should return NULL */
  context_t ctx = context_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "Tuple");
  EXPECT_EQ(be, nullptr);
  context_dispose(ctx);
}

TEST_F(dt_builtin, tuple_declaration) {
  /* Tuple is now a native type — <i32, f64> syntax, not builtin type declaration */
  const char *src = "var t: <i32, f64> = undefined;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_instantiation_two_fields) {
  const char *src = BUILTIN_ASSERT
    "var t: <i32, f64> = undefined;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_init_list) {
  const char *src = BUILTIN_ASSERT
    "var t: <i32, f64> = .<i32, f64>{1, 2.0};\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript) {
  const char *src = BUILTIN_ASSERT
    "test \"tuple_get\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  assert(t[0] == 1);\n"
    "  assert(t[1] == 2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (context_get_error_count(r.ctx) > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_single_field) {
  const char *src = BUILTIN_ASSERT
    "test \"tuple_single\" {\n"
    "  var t: <i32> = .<i32>{42};\n"
    "  assert(t[0] == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_three_fields) {
  const char *src = BUILTIN_ASSERT
    "test \"tuple_three\" {\n"
    "  var t: <i32, f64, bool> = .<i32, f64, bool>{1, 2.0, true};\n"
    "  assert(t[0] == 1);\n"
    "  assert(t[1] == 2.0);\n"
    "  assert(t[2] == true);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_same_instantiation_dedup) {
  /* <i32, f64> used twice should be the same type */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_dedup\" {\n"
    "  var t1: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  var t2: <i32, f64> = .<i32, f64>{3, 4.0};\n"
    "  assert(t1[0] == 1);\n"
    "  assert(t2[0] == 3);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript_out_of_range) {
  /* Tuple subscript with out-of-range index should error */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_sub_oob\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  t[5];\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript_non_tuple) {
  /* Subscript on non-tuple type should error */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_sub_nontuple\" {\n"
    "  var x: i32 = 5;\n"
    "  x[0];\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript_assign) {
  /* Tuple subscript assignment modifies field in-place */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_set\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  t[0] = 10;\n"
    "  assert(t[0] == 10);\n"
    "  assert(t[1] == 2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (context_get_error_count(r.ctx) > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Builtin length function ===== */

TEST_F(dt_builtin, table_lookup_length) {
  context_t ctx = context_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "length");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  context_dispose(ctx);
}

TEST_F(dt_builtin, length_declaration) {
  const char *src = "builtin func length[T](list: T): u64;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);

  struct symbol *sym = scope_lookup(r.ctx->global_scope, "length");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_array) {
  const char *src = BUILTIN_ASSERT
    "builtin func length[T](list: T): u64;\n"
    "test \"length_array\" {\n"
    "  var arr: [5]i32 = .{0, 0, 0, 0, 0};\n"
    "  assert(length(arr) == 5);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_tuple) {
  const char *src = BUILTIN_ASSERT
    "builtin func length[T](list: T): u64;\n"
    "test \"length_tuple\" {\n"
    "  var t: <i32, f64, bool> = .<i32, f64, bool>{1, 2.0, true};\n"
    "  assert(length(t) == 3);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_non_array_tuple_error) {
  /* length on a non-array, non-tuple should error */
  const char *src = BUILTIN_ASSERT
    "builtin func length[T](list: T): u64;\n"
    "test \"length_err\" {\n"
    "  var x: i32 = 5;\n"
    "  length(x);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== assert only allowed in test blocks ===== */

TEST_F(dt_builtin, assert_outside_test_block_error) {
  /* assert in comptime block (not test) should be an error */
  const char *src = BUILTIN_ASSERT
    "comptime if (true) {\n"
    "  assert(true);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, assert_in_test_block_ok) {
  /* assert inside test block is allowed */
  const char *src = BUILTIN_ASSERT
    "test \"assert_in_test\" {\n"
    "  assert(true);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, assert_failure_continues_in_test) {
  /* assert failure does NOT abort the test block — subsequent stmts run */
  const char *src = BUILTIN_ASSERT
    "test \"assert_continue\" {\n"
    "  assert(false);\n"
    "  var x: i32 = 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  /* assert(false) reports error but block continues — no fatal */
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  EXPECT_FALSE(r.ctx->fatal_error);
  /* test should be counted as failed */
  EXPECT_GT(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}
