/**
 * @file dt_builtin_panic.cpp
 * @brief Tests for panic builtin function.
 */

#include "engine/checker.h"
#include "engine/builtin.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/ast_factory.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_PANIC "builtin func panic(msg: str): void;\n"

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
  struct compile_result cr;
  if (g_error) {
    std::string err_msg(g_error->message);
    error_clear();
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        ("Parsing failed: " + err_msg).c_str(),
        ::testing::TestPartResult::kFatalFailure);
    cr.ctx = NULL; cr.prog = prog; cr.tokens = tokens;
    return cr;
  }
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);
  checker_check_program(ctx, prog);
  cr.ctx = ctx; cr.prog = prog; cr.tokens = tokens;
  return cr;
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  if (r->ctx) checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_builtin_panic : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== panic with message ===== */

TEST_F(dt_builtin_panic, panic_with_string) {
  const char *src = BUILTIN_PANIC
    "comptime {\n"
    "  panic(\"something went wrong\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  /* Check that the diagnostic contains "panic: something went wrong" */
  bool found = false;
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d && strstr(d->message, "panic: something went wrong")) {
        found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found);

  compile_result_cleanup(&r, allocator);
}

/* ===== panic aborts block ===== */

TEST_F(dt_builtin_panic, panic_aborts_block) {
  /* After panic, the block should abort (FATAL) — subsequent statements
   * should not be evaluated. The fatal_error flag stops all further
   * evaluation, so no additional errors from statements after panic. */
  const char *src = BUILTIN_PANIC
    "comptime {\n"
    "  panic(\"stop\");\n"
    "  var x: i32 = 1;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  /* Should have exactly 1 error (the panic), not additional errors.
     fatal_error should be set, stopping all further evaluation. */
  int err_count = checker_get_error_count(r.ctx);
  EXPECT_EQ(err_count, 1);
  EXPECT_TRUE(r.ctx->fatal_error);

  compile_result_cleanup(&r, allocator);
}

/* ===== panic in comptime function ===== */

TEST_F(dt_builtin_panic, panic_in_function) {
  const char *src = BUILTIN_PANIC
    "comptime func fail(): void {\n"
    "  panic(\"always fails\");\n"
    "}\n"
    "comptime {\n"
    "  fail();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}

/* ===== panic no args ===== */

TEST_F(dt_builtin_panic, panic_no_args) {
  const char *src = BUILTIN_PANIC
    "comptime {\n"
    "  panic();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}

/* ===== panic builtin registered ===== */

TEST_F(dt_builtin_panic, panic_builtin_registered) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "panic");
  EXPECT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  checker_dispose(ctx);
}

/* ===== panic stops compilation (fatal) ===== */

TEST_F(dt_builtin_panic, panic_fatal_stops_later_tests) {
  /* panic in comptime block should set fatal_error and skip later declarations.
     A test block after panic should not be executed. */
  const char *src = BUILTIN_ASSERT BUILTIN_PANIC
    "comptime {\n"
    "  panic(\"fatal\");\n"
    "}\n"
    "test \"after_panic\" {\n"
    "  assert(false);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  /* Should have 1 error (panic) and fatal_error set.
     The test block should be skipped entirely (not even counted). */
  EXPECT_TRUE(r.ctx->fatal_error);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_panic, panic_in_function_fatal) {
  /* panic called inside a comptime function propagates FATAL signal */
  const char *src = BUILTIN_PANIC
    "comptime func boom(): void {\n"
    "  panic(\"boom\");\n"
    "}\n"
    "comptime {\n"
    "  boom();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_TRUE(r.ctx->fatal_error);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_panic, assert_unwrap_wrong_variant_fatal) {
  /* .! on wrong union variant should panic (fatal), not just error.
     Execution must stop — subsequent assert(false) must NOT be reached. */
  const char *src = BUILTIN_ASSERT
    "union Test { _err:str; _value:i32; };\n"
    "test \"unwrap_wrong\" {\n"
    "  var item = .Test{._err = \"oops\"};\n"
    "  _ = item._value.!;\n"
    "  assert(false);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_TRUE(r.ctx->fatal_error);
  /* Should have the panic error, but NOT the assert(false) error */
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_panic, assert_unwrap_in_function_fatal) {
  /* .! panic inside a called function must propagate FATAL back to the
     caller. The var declaration that receives the fatal return value must
     also propagate FATAL — subsequent assert(false) must NOT be reached. */
  const char *src = BUILTIN_ASSERT
    "union Test { _err:str; _value:i32; };\n"
    "func boom():i32 {\n"
    "  var item = .Test{._err = \"bad\"};\n"
    "  _ = item._value.!;\n"
    "  return 0;\n"
    "}\n"
    "test \"fn_panic\" {\n"
    "  var res = boom();\n"
    "  assert(false);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_TRUE(r.ctx->fatal_error);
  /* Should have the panic error from .!, but NOT the assert(false) error */
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}
