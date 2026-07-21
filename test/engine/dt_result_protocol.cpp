/**
 * @file dt_result_protocol.cpp
 * @brief Tests for .? and .! with Result protocol (isError/value/error).
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

#define BUILTIN_ASSERT "builtin func assert(cond: bool): void;\n"
#define BUILTIN_UNIONIS "builtin func unionIs[T](v: T): bool;\n"

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

class dt_result_protocol : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== .? on union member access ===== */

TEST_F(dt_result_protocol, try_union_field_value) {
  /* u.value.? on a union where value is the active variant should return value */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"try_field_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.value.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, try_union_field_error_propagate) {
  /* u.value.? on a union where value is NOT active should propagate error */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"try_field_error\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.value.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, try_union_err_field_active) {
  /* u.err.? on a union where err IS active should return the err value */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"try_err_field\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.err.?;\n"
    "  assert(v == \"fail\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, try_union_generic_field) {
  /* u.value.? on a generic union */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result[V, E] { value: V; err: E; }\n"
    "test \"try_generic\" {\n"
    "  var r = .Result[i32, str]{.value = 42};\n"
    "  var v = r.value.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .! on union member access ===== */

TEST_F(dt_result_protocol, assert_union_field_value) {
  /* u.value.! on a union where value is active should return value */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"assert_field_value\" {\n"
    "  var r = .Result{.value = 42};\n"
    "  var v = r.value.!;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, assert_union_field_panic) {
  /* u.value.! on a union where value is NOT active should panic */
  const char *src = BUILTIN_ASSERT BUILTIN_UNIONIS
    "union Result { value: i32; err: str; }\n"
    "test \"assert_field_panic\" {\n"
    "  var r = .Result{.err = \"fail\"};\n"
    "  var v = r.value.!;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .? on pointer ===== */

TEST_F(dt_result_protocol, try_pointer_deref) {
  /* ptr.? should dereference if non-null */
  const char *src = BUILTIN_ASSERT
    "test \"try_ptr\" {\n"
    "  var x: i32 = 10;\n"
    "  var p = x.&;\n"
    "  var v = p.?;\n"
    "  assert(v == 10);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, try_pointer_null) {
  /* ptr.? on null pointer should error */
  const char *src = BUILTIN_ASSERT
    "test \"try_null\" {\n"
    "  var p: *i32 = null;\n"
    "  var v = p.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== .! on pointer ===== */

TEST_F(dt_result_protocol, assert_pointer_deref) {
  /* ptr.! should dereference if non-null */
  const char *src = BUILTIN_ASSERT
    "test \"assert_ptr\" {\n"
    "  var x: i32 = 10;\n"
    "  var p = x.&;\n"
    "  var v = p.!;\n"
    "  assert(v == 10);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== type errors ===== */

TEST_F(dt_result_protocol, try_on_non_union_non_pointer) {
  /* .? on a plain i32 should be a type error */
  const char *src = BUILTIN_ASSERT
    "test \"try_plain\" {\n"
    "  var x: i32 = 5;\n"
    "  var v = x.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, assert_on_non_union_non_pointer) {
  /* .! on a plain i32 should be a type error */
  const char *src = BUILTIN_ASSERT
    "test \"assert_plain\" {\n"
    "  var x: i32 = 5;\n"
    "  var v = x.!;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Result protocol via isError/value/Error methods on struct ===== */

/* A custom Result type implemented as a struct with instance methods.
 * This tests the general Result protocol, not the built-in union mechanism. */

TEST_F(dt_result_protocol, struct_result_try_value) {
  /* struct with isError/value/error: .? should call isError(), then value() */
  const char *src = BUILTIN_ASSERT
    "struct MyResult {\n"
    "  _val: i32;\n"
    "  _err: str;\n"
    "  _isErr: bool;\n"
    "  func isError(self: *MyResult): bool { return self._isErr; }\n"
    "  func value(self: *MyResult): i32 { return self._val; }\n"
    "  func error(self: *MyResult): str { return self._err; }\n"
    "}\n"
    "test \"struct_result_try\" {\n"
    "  var r = .MyResult{._val = 42, ._err = \"\", ._isErr = false};\n"
    "  var v = r.?;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, struct_result_try_error_propagate) {
  /* .? on struct Result in error state should propagate error */
  const char *src = BUILTIN_ASSERT
    "struct MyResult {\n"
    "  _val: i32;\n"
    "  _err: str;\n"
    "  _isErr: bool;\n"
    "  func isError(self: *MyResult): bool { return self._isErr; }\n"
    "  func value(self: *MyResult): i32 { return self._val; }\n"
    "  func error(self: *MyResult): str { return self._err; }\n"
    "}\n"
    "test \"struct_result_try_err\" {\n"
    "  var r = .MyResult{._val = 0, ._err = \"fail\", ._isErr = true};\n"
    "  var v = r.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, struct_result_assert_value) {
  /* .! on struct Result in value state should return value */
  const char *src = BUILTIN_ASSERT
    "struct MyResult {\n"
    "  _val: i32;\n"
    "  _err: str;\n"
    "  _isErr: bool;\n"
    "  func isError(self: *MyResult): bool { return self._isErr; }\n"
    "  func value(self: *MyResult): i32 { return self._val; }\n"
    "  func error(self: *MyResult): str { return self._err; }\n"
    "}\n"
    "test \"struct_result_assert\" {\n"
    "  var r = .MyResult{._val = 42, ._err = \"\", ._isErr = false};\n"
    "  var v = r.!;\n"
    "  assert(v == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, struct_result_assert_panic) {
  /* .! on struct Result in error state should panic */
  const char *src = BUILTIN_ASSERT
    "struct MyResult {\n"
    "  _val: i32;\n"
    "  _err: str;\n"
    "  _isErr: bool;\n"
    "  func isError(self: *MyResult): bool { return self._isErr; }\n"
    "  func value(self: *MyResult): i32 { return self._val; }\n"
    "  func error(self: *MyResult): str { return self._err; }\n"
    "}\n"
    "test \"struct_result_assert_panic\" {\n"
    "  var r = .MyResult{._val = 0, ._err = \"fail\", ._isErr = true};\n"
    "  var v = r.!;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);

  /* Verify the diagnostic contains "panic" */
  bool found_panic = false;
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d && strstr(d->message, "panic:")) {
        found_panic = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_panic);

  compile_result_cleanup(&r, allocator);
}

/* ===== Missing method detection ===== */

TEST_F(dt_result_protocol, try_missing_value_method) {
  /* .? on struct with isError but no value should be a type error */
  const char *src = BUILTIN_ASSERT
    "struct BadResult {\n"
    "  _isErr: bool;\n"
    "  func isError(self: *BadResult): bool { return self._isErr; }\n"
    "}\n"
    "test \"try_no_value\" {\n"
    "  var r = .BadResult{._isErr = false};\n"
    "  var v = r.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_result_protocol, try_missing_error_method) {
  /* .? on struct with isError and value but no error should be a type error */
  const char *src = BUILTIN_ASSERT
    "struct BadResult {\n"
    "  _val: i32;\n"
    "  _isErr: bool;\n"
    "  func isError(self: *BadResult): bool { return self._isErr; }\n"
    "  func value(self: *BadResult): i32 { return self._val; }\n"
    "}\n"
    "test \"try_no_error\" {\n"
    "  var r = .BadResult{._val = 42, ._isErr = false};\n"
    "  var v = r.?;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
