#include "engine/checker.h"
#include "engine/builtin.h"
#include "engine/symbol.h"
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
