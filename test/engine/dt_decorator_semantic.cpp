#include "engine/checker.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_TYPENAME "builtin func typename[T](): str;\n"

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

/* ===== test fixture ===== */

class dt_decorator_semantic : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== typename builtin ===== */

TEST_F(dt_decorator_semantic, typename_builtin_i32) {
  const char *src = BUILTIN_TYPENAME
      "comptime var name: str = typename[i32]();\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  comptime_value_t v = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc, "name");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "i32");

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, typename_builtin_user_type) {
  const char *src = BUILTIN_TYPENAME
      "struct Point { x: i32; y: i32; }\n"
      "comptime var name: str = typename[Point]();\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  comptime_value_t v = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc, "name");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "Point");

  compile_result_cleanup(&r, allocator);
}

/* ===== decorator: comptime var (value transform) ===== */

TEST_F(dt_decorator_semantic, comptime_var_decorator_basic) {
  /* [[double]] comptime var x: i32 = 5; → double(5) = 10 */
  const char *src =
      "comptime func double(x: i32): i32 { return x * 2; }\n"
      "[[double]] comptime var x: i32 = 5;\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  comptime_value_t v = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 10);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, comptime_var_decorator_chain) {
  /* [[double]] [[double]] comptime var x: i32 = 3; → double(double(3)) = 12 */
  const char *src =
      "comptime func double(x: i32): i32 { return x * 2; }\n"
      "[[double]] [[double]] comptime var x: i32 = 3;\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  comptime_value_t v = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 12);

  compile_result_cleanup(&r, allocator);
}

/* ===== decorator: error cases ===== */

TEST_F(dt_decorator_semantic, unknown_decorator_error) {
  const char *src =
      "[[nonExistent]] comptime var x: i32 = 5;\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, decorator_not_function_error) {
  /* [[x]] where x is a variable, not a function */
  const char *src =
      "comptime var x: i32 = 42;\n"
      "[[x]] comptime var y: i32 = 5;\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, decorator_factory_not_function_error) {
  /* [[x(1)]] where x returns i32, not a function */
  const char *src =
      "comptime func x(a: i32): i32 { return a; }\n"
      "[[x(1)]] comptime var y: i32 = 5;\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== decorator: type (no crash) ===== */

TEST_F(dt_decorator_semantic, type_decorator_unknown_func) {
  /* [[unknownFunc]] struct Foo — decorator func not found, should error */
  const char *src =
      "[[unknownFunc]] struct Foo {}\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== decorator: function closure semantics ===== */

TEST_F(dt_decorator_semantic, func_decorator_basic) {
  /* [[identity]] func add(a: i32, b: i32): i32 { return a + b; }
   * The decorator returns the original function unchanged.
   * Original should be saved as __original_add in comptime env. */
  const char *src =
      "builtin func assert(condition: bool): void;\n"
      "comptime func identity(f) { return f; }\n"
      "[[identity]] func add(a: i32, b: i32): i32 { return a + b; }\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  /* __original_add should be bound in comptime env */
  comptime_value_t orig = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc,
      "__original_add");
  ASSERT_NE(orig, nullptr);
  EXPECT_EQ(orig->kind, COMPTIME_VALUE_FUNCTION);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, func_decorator_original_preserved) {
  /* Decorator that returns the original function unchanged.
   * Verify __original_<name> is bound and the main binding still works. */
  const char *src =
      "builtin func assert(condition: bool): void;\n"
      "comptime func identity(f) { return f; }\n"
      "[[identity]] func greet(): str { return \"hello\"; }\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  /* Both __original_greet and greet should exist in comptime env */
  comptime_value_t orig = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc,
      "__original_greet");
  ASSERT_NE(orig, nullptr);
  EXPECT_EQ(orig->kind, COMPTIME_VALUE_FUNCTION);

  comptime_value_t wrapped = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc,
      "greet");
  ASSERT_NE(wrapped, nullptr);
  EXPECT_EQ(wrapped->kind, COMPTIME_VALUE_FUNCTION);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_decorator_semantic, func_decorator_chain) {
  /* Two decorators on the same function.
   * First decorator saves original as __original_<name>.
   * Second decorator sees the first decorator's result as the current binding. */
  const char *src =
      "builtin func assert(condition: bool): void;\n"
      "comptime func identity(f) { return f; }\n"
      "[[identity]] [[identity]] func compute(): i32 { return 42; }\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  /* __original_compute should exist (saved by first decorator) */
  comptime_value_t orig = comptime_env_lookup_value(
      r.ctx->comptime_eval->global_env, r.ctx->comptime_eval->valloc,
      "__original_compute");
  ASSERT_NE(orig, nullptr);
  EXPECT_EQ(orig->kind, COMPTIME_VALUE_FUNCTION);

  compile_result_cleanup(&r, allocator);
}
