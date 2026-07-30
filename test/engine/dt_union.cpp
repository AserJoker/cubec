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

class dt_union : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_union, union_init_named) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"init\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(r.value.! == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, cunion_init_named) {
  const char *src = BUILTIN_ASSERT
    "cunion Data { a: i32; b: f64; }\n"
    "test \"init\" {\n"
    "  var d = .Data{.a = 10};\n"
    "  assert(d.a == 10);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, unionis_true) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"is_true\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(r is i32);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  if (r.ctx->error_count > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, unionis_false) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"is_false\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(!(r is str));\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  if (r.ctx->error_count > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, union_tag_updated_on_write) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"tag_update\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(r is i32);\n"
    "  r.err = \"hello\";\n"
    "  assert(r is str);\n"
    "  assert(!(r is i32));\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  if (r.ctx->error_count > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, union_positional_init_first_field) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"pos_init\" {\n"
    "  var r = .Result{42};\n"
    "  assert(r.value.! == 42);\n"
    "  assert(r is i32);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  if (r.ctx->error_count > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .? (try / error propagation) ===== */

TEST_F(dt_union, try_union_value_variant) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"try_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.value.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, try_union_error_propagate) {
  /* .? on wrong variant should produce a diagnostic */
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"try_error\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.value.?;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, try_union_generic) {
  const char *src = BUILTIN_ASSERT
    "union Result[V, E] { value: V; err: E; }\n"
    "test \"try_generic\" {\n"
    "  var r = .Result[i32, str]{.value = 42};\n"
    "  var v = r.value.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  if (r.ctx->error_count > 0) {
    size_t dc = diagnostic_list_get_size(r.ctx->diagnostics);
    for (size_t i = 0; i < dc; i++) {
      struct diagnostic *d = diagnostic_list_get(r.ctx->diagnostics, i);
      if (d && d->severity == DIAGNOSTIC_ERROR)
        printf("  DIAG[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, try_non_union_error) {
  /* .? on non-union, non-pointer type should be a type error */
  const char *src = BUILTIN_ASSERT
    "test \"try_non_union\" {\n"
    "  var x: i32 = 5;\n"
    "  var v = x.?;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .! (assert / panic) ===== */

TEST_F(dt_union, assert_union_value) {
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"assert_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.value.!;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, assert_union_panic) {
  /* .! on wrong variant should produce a panic diagnostic */
  const char *src = BUILTIN_ASSERT
    "union Result { value: i32; err: str; }\n"
    "test \"assert_panic\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.value.!;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, assert_non_union_error) {
  /* .! on non-union, non-pointer type should be a type error */
  const char *src = BUILTIN_ASSERT
    "test \"assert_non_union\" {\n"
    "  var x: i32 = 5;\n"
    "  var v = x.!;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
