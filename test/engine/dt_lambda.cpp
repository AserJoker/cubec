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

class dt_lambda : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== anonymous function ===== */

TEST_F(dt_lambda, basic_lambda) {
  const char *src = BUILTIN_ASSERT
    "test \"lambda_basic\" {\n"
    "  var f = func(x: i32): i32 { return x + 1; };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, lambda_no_params) {
  const char *src = BUILTIN_ASSERT
    "test \"lambda_no_params\" {\n"
    "  var f = func(): i32 { return 42; };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, lambda_with_capture) {
  const char *src = BUILTIN_ASSERT
    "test \"lambda_capture\" {\n"
    "  var y = 10;\n"
    "  var f = func|y|(x: i32): i32 { return x + y; };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, lambda_call_inline) {
  const char *src = BUILTIN_ASSERT
    "test \"lambda_call\" {\n"
    "  var result = func(x: i32): i32 { return x * 2; }(21);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, statement_func_with_capture) {
  /* Local named function with capture list: func |x| name() { ... } */
  const char *src = BUILTIN_ASSERT
    "test \"closure\" {\n"
    "  var x = 1;\n"
    "  func |x| testfn(): void {\n"
    "    x = x + 1;\n"
    "  }\n"
    "  testfn();\n"
    "  assert(x == 1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "error_count=" << r.ctx->error_count;
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, local_struct) {
  /* Struct declared inside a test block */
  const char *src = BUILTIN_ASSERT
    "test \"local_struct\" {\n"
    "  struct Point { x: i32; y: i32; }\n"
    "  var p = .Point { .x = 1, .y = 2 };\n"
    "  assert(p.x == 1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "error_count=" << r.ctx->error_count;
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, local_enum) {
  /* Enum declared inside a test block */
  const char *src = BUILTIN_ASSERT
    "test \"local_enum\" {\n"
    "  enum Color { Red, Green, Blue }\n"
    "  var c: Color = Color.Red;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "error_count=" << r.ctx->error_count;
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, local_union) {
  /* Union declared inside a test block */
  const char *src = BUILTIN_ASSERT
    "test \"local_union\" {\n"
    "  union Value { i32: i32; f64: f64; }\n"
    "  var v: Value = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "error_count=" << r.ctx->error_count;
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, local_cunion) {
  /* C-style union declared inside a test block */
  const char *src = BUILTIN_ASSERT
    "test \"local_cunion\" {\n"
    "  cunion Data { i32: i32; f64: f64; }\n"
    "  var d: Data = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "error_count=" << r.ctx->error_count;
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, expr_func_return_exhaustiveness) {
  /* Expression function with non-void return type must return on all paths */
  const char *src = BUILTIN_ASSERT
    "test \"expr_func_return\" {\n"
    "  var f = func(x: i32): i32 { if (x > 0) { return x; } };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "should error: missing return on all paths";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, expr_func_void_no_return_needed) {
  /* Expression function with void return type does not require return */
  const char *src = BUILTIN_ASSERT
    "test \"expr_func_void\" {\n"
    "  var f = func(x: i32): void { };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_lambda, local_enum_non_numeric_value_error) {
  /* Local enum with non-numeric value should error (consistent with global) */
  const char *src = BUILTIN_ASSERT
    "test \"local_enum_non_numeric\" {\n"
    "  enum E { A = \"hello\" }\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "should error: enum value must be integer literal";
  compile_result_cleanup(&r, allocator);
}
