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
  EXPECT_EQ(be->kind, BUILTIN_FUNC);
  EXPECT_EQ(be->dispatch, BUILTIN_DISPATCH_ASSERT);
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
  /* 'assert' is a builtin func, not a builtin var */
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

/* ===== Builtin Tuple type ===== */

TEST_F(dt_builtin, table_lookup_tuple) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "Tuple");
  ASSERT_NE(be, nullptr);
  EXPECT_EQ(be->kind, BUILTIN_TYPE);
  EXPECT_EQ(be->dispatch, BUILTIN_DISPATCH_TUPLE);
  checker_dispose(ctx);
}

TEST_F(dt_builtin, tuple_declaration) {
  const char *src = "builtin type Tuple[...Args];\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  struct symbol *sym = scope_lookup(r.ctx->global_scope, "Tuple");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_instantiation_two_fields) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_inst\" {\n"
    "  var t: Tuple[i32, f64];\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_init_list) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_init\" {\n"
    "  var t = .Tuple[i32, f64]{1, 2.0};\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_subscript_read) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_sub\" {\n"
    "  var t = .Tuple[i32, f64]{1, 2.0};\n"
    "  assert(t[0] == 1);\n"
    "  assert(t[1] == 2.0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_single_field) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_single\" {\n"
    "  var t = .Tuple[i32]{42};\n"
    "  assert(t[0] == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_three_fields) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_three\" {\n"
    "  var t = .Tuple[i32, f64, bool]{1, 2.0, true};\n"
    "  assert(t[0] == 1);\n"
    "  assert(t[1] == 2.0);\n"
    "  assert(t[2] == true);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, tuple_same_instantiation_dedup) {
  /* Tuple[i32, f64] instantiated twice should be the same type */
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "test \"tuple_dedup\" {\n"
    "  var t1 = .Tuple[i32, f64]{1, 2.0};\n"
    "  var t2 = .Tuple[i32, f64]{3, 4.0};\n"
    "  assert(t1[0] == 1);\n"
    "  assert(t2[0] == 3);\n"
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
  EXPECT_EQ(be->kind, BUILTIN_FUNC);
  EXPECT_EQ(be->dispatch, BUILTIN_DISPATCH_LENGTH);
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
    "  var arr: [5]i32;\n"
    "  assert(length(arr) == 5);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin, length_tuple) {
  const char *src = BUILTIN_ASSERT
    "builtin type Tuple[...Args];\n"
    "builtin func length[T](list: T): u64;\n"
    "test \"length_tuple\" {\n"
    "  var t = .Tuple[i32, f64, bool]{1, 2.0, true};\n"
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
