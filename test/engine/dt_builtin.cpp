#include "engine/checker.h"
#include "engine/builtin.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_GET "builtin func getTupleItem[N: u64, ...Args](tuple: <...Args>): Args[N];\n"
#define BUILTIN_SET "builtin func setTupleItem[N: u64, ...Args](tuple: <...Args>, value: Args[N]): void;\n"

struct compile_result {
  checker_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_source(allocator_t allocator,
                                            const char *source) {
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(allocator, tokens, &pos, "test.cubec");
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);

  /* If parsing failed, g_error is set — convert to a diagnostic */
  if (g_error) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         (location_t){0}, "%s", g_error->message);
    ctx->error_count++;
    error_clear();
  }

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_builtin : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== Builtin table API ===== */

TEST_F(dt_builtin, table_create_dispose) {
  checker_t ctx = checker_create(allocator);
  ASSERT_NE(ctx->builtin_table, nullptr);
  checker_dispose(ctx);
}

TEST_F(dt_builtin, table_lookup_assert) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "assert");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  checker_dispose(ctx);
}

TEST_F(dt_builtin, table_lookup_unknown) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "nonexistent");
  EXPECT_EQ(be, nullptr);
  checker_dispose(ctx);
}

/* ===== Builtin declaration validation ===== */

TEST_F(dt_builtin, assert_declared_correctly) {
  const char *src = "builtin func assert(condition: bool): void;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* Symbol should be marked as builtin */
  struct symbol *sym = scope_lookup(r.ctx->global_scope, "assert");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, unknown_builtin_declaration) {
  const char *src = "builtin func notexist(x: i32): void;\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_signature_mismatch) {
  /* assert expects (bool): void, not (i32): void */
  const char *src = "builtin func assert(condition: i32): void;\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_kind_mismatch) {
  /* 'assert' is a builtin func, declaring as var should error */
  const char *src = "builtin var assert: bool;\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, builtin_assert_e2e) {
  const char *src =
    "builtin func assert(condition: bool): void;\n"
    "test \"builtin_assert\" { assert(true); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->test_count, 1);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, non_builtin_func_not_marked) {
  const char *src =
    "builtin func assert(condition: bool): void;\n"
    "func foo(): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

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
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "Tuple");
  EXPECT_EQ(be, nullptr);
  checker_dispose(ctx);
}

TEST_F(dt_builtin, tuple_declaration) {
  /* Tuple is now a native type — <i32, f64> syntax, not builtin type declaration */
  const char *src = "var t: <i32, f64> = undefined;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_instantiation_two_fields) {
  const char *src = BUILTIN_ASSERT
    "var t: <i32, f64> = undefined;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_init_list) {
  const char *src = BUILTIN_ASSERT
    "var t: <i32, f64> = .<i32, f64>{1, 2.0};\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_getTupleItem) {
  const char *src = BUILTIN_ASSERT BUILTIN_GET
    "test \"tuple_get\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  assert(getTupleItem[0](t) == 1);\n"
    "  assert(getTupleItem[1](t) == 2.0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_single_field) {
  const char *src = BUILTIN_ASSERT BUILTIN_GET
    "test \"tuple_single\" {\n"
    "  var t: <i32> = .<i32>{42};\n"
    "  assert(getTupleItem[0](t) == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_three_fields) {
  const char *src = BUILTIN_ASSERT BUILTIN_GET
    "test \"tuple_three\" {\n"
    "  var t: <i32, f64, bool> = .<i32, f64, bool>{1, 2.0, true};\n"
    "  assert(getTupleItem[0](t) == 1);\n"
    "  assert(getTupleItem[1](t) == 2.0);\n"
    "  assert(getTupleItem[2](t) == true);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_same_instantiation_dedup) {
  /* <i32, f64> used twice should be the same type */
  const char *src = BUILTIN_ASSERT BUILTIN_GET
    "test \"tuple_dedup\" {\n"
    "  var t1: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  var t2: <i32, f64> = .<i32, f64>{3, 4.0};\n"
    "  assert(getTupleItem[0](t1) == 1);\n"
    "  assert(getTupleItem[0](t2) == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript_error) {
  /* t[0] should be an error — must use getTupleItem[0](t) */
  const char *src = BUILTIN_ASSERT
    "test \"tuple_sub_err\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  t[0];\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_setTupleItem) {
  /* setTupleItem modifies tuple field in-place */
  const char *src = BUILTIN_ASSERT BUILTIN_GET BUILTIN_SET
    "test \"tuple_set\" {\n"
    "  var t: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  setTupleItem[0](t, 10);\n"
    "  assert(getTupleItem[0](t) == 10);\n"
    "  assert(getTupleItem[1](t) == 2.0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Builtin length function ===== */

TEST_F(dt_builtin, table_lookup_length) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "length");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  checker_dispose(ctx);
}

TEST_F(dt_builtin, length_declaration) {
  const char *src = "builtin func length[T](list: T): u64;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  struct symbol *sym = scope_lookup(r.ctx->global_scope, "length");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_array) {
  const char *src = BUILTIN_ASSERT
    "builtin func length[T](list: T): u64;\n"
    "test \"length_array\" {\n"
    "  var arr: [5]i32 = undefined;\n"
    "  assert(length(arr) == 5);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_tuple) {
  const char *src = BUILTIN_ASSERT
    "builtin func length[T](list: T): u64;\n"
    "test \"length_tuple\" {\n"
    "  var t: <i32, f64, bool> = .<i32, f64, bool>{1, 2.0, true};\n"
    "  assert(length(t) == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
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
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
