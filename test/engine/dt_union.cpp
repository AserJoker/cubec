#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_UNIONIS "builtin func unionIs[T,K](obj:K):bool;\n"

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

/* ===== test fixture ===== */

class dt_union : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

TEST_F(dt_union, union_init_named) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"init\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(r.value == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
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
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, unionis_true) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"is_true\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(unionIs[i32](r));\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, unionis_false) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"is_false\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(!unionIs[str](r));\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, union_tag_updated_on_write) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"tag_update\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  assert(unionIs[i32](r));\n"
    "  r.err = \"hello\";\n"
    "  assert(unionIs[str](r));\n"
    "  assert(!unionIs[i32](r));\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, union_positional_init_first_field) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"pos_init\" {\n"
    "  var r = .Result{42};\n"
    "  assert(r.value == 42);\n"
    "  assert(unionIs[i32](r));\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .? (try / error propagation) ===== */

TEST_F(dt_union, try_union_value_variant) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"try_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, try_union_error_propagate) {
  /* .? on error variant should produce a diagnostic */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"try_error\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, try_union_generic) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result[V, E] { value: V; err: E; }\n"
    "test \"try_generic\" {\n"
    "  var r = .Result[i32, str]{.value = 42};\n"
    "  var v = r.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
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
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .! (assert / panic) ===== */

TEST_F(dt_union, assert_union_value) {
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"assert_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.!;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_union, assert_union_panic) {
  /* .! on error variant should produce a panic diagnostic */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"assert_panic\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.!;\n"
    "}\n";
  auto r = compile_source(allocator, src);
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
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
