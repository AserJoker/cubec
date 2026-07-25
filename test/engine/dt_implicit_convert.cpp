#include "engine/context.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_CAST "builtin func cast[T, K](value: K): T;\n"

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

class dt_implicit_convert : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== int -> float NOT allowed ===== */

TEST_F(dt_implicit_convert, int_to_float_rejected) {
  const char *src = BUILTIN_ASSERT
    "test \"int_to_float\" {\n"
    "  var x: f64 = 42;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "int->float implicit conversion should be rejected";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_implicit_convert, int_to_float_explicit_cast_ok) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"int_to_float_cast\" {\n"
    "  var x: f64 = cast[f64](42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_implicit_convert, float_widening_ok) {
  const char *src = BUILTIN_ASSERT
    "test \"float_widen\" {\n"
    "  var x: f64 = 1.0f32;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_implicit_convert, int_widening_ok) {
  const char *src = BUILTIN_ASSERT
    "test \"int_widen\" {\n"
    "  var x: i64 = 42;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== slice -> pointer NOT allowed ===== */

TEST_F(dt_implicit_convert, slice_to_pointer_rejected) {
  const char *src = BUILTIN_ASSERT
    "test \"slice_to_ptr\" {\n"
    "  var s: []i32;\n"
    "  var p: *i32 = s;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "[]T->*T implicit conversion should be rejected";
  compile_result_cleanup(&r, allocator);
}

/* ===== array -> slice NOT allowed (design: no implicit decay) ===== */

TEST_F(dt_implicit_convert, array_to_slice_rejected) {
  const char *src = BUILTIN_ASSERT
    "test \"array_to_slice\" {\n"
    "  var arr: [3]i32;\n"
    "  var s: []i32 = arr;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "[3]i32->[]i32 implicit conversion should be rejected";
  compile_result_cleanup(&r, allocator);
}

/* ===== nil -> pointer IS allowed ===== */

TEST_F(dt_implicit_convert, nil_to_pointer_ok) {
  const char *src = BUILTIN_ASSERT
    "test \"nil_to_ptr\" {\n"
    "  var p: *i32 = nil;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
