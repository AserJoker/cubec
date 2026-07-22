/**
 * @file dt_foreach.cpp
 * @brief Tests for foreach with iterator protocol (next() → {value, done}).
 */

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

#define BUILTIN_ASSERT "builtin func assert(cond: bool): void;\n"

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

class dt_foreach : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* The iterator protocol uses a struct with named fields 'value' and 'done'
 * rather than a tuple, since tuple fields are positional (_0, _1). */

/* ===== Basic iterator protocol ===== */

TEST_F(dt_foreach, range_iterator) {
  const char *src = BUILTIN_ASSERT
    "struct IterResult {\n"
    "  value: i32;\n"
    "  done: bool;\n"
    "}\n"
    "struct RangeIter {\n"
    "  current: i32;\n"
    "  end: i32;\n"
    "  func next(self: *RangeIter): IterResult {\n"
    "    if (self.current >= self.end) {\n"
    "      return .IterResult{.value = 0, .done = true};\n"
    "    }\n"
    "    var v = self.current;\n"
    "    self.current = self.current + 1;\n"
    "    return .IterResult{.value = v, .done = false};\n"
    "  }\n"
    "}\n"
    "test \"range_iter\" {\n"
    "  var sum: i32 = 0;\n"
    "  var it = .RangeIter{.current = 0, .end = 3};\n"
    "  foreach (x of it) {\n"
    "    sum = sum + x;\n"
    "  }\n"
    "  assert(sum == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d) printf("  diag[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_foreach, range_iterator_empty) {
  const char *src = BUILTIN_ASSERT
    "struct IterResult {\n"
    "  value: i32;\n"
    "  done: bool;\n"
    "}\n"
    "struct RangeIter {\n"
    "  current: i32;\n"
    "  end: i32;\n"
    "  func next(self: *RangeIter): IterResult {\n"
    "    if (self.current >= self.end) {\n"
    "      return .IterResult{.value = 0, .done = true};\n"
    "    }\n"
    "    var v = self.current;\n"
    "    self.current = self.current + 1;\n"
    "    return .IterResult{.value = v, .done = false};\n"
    "  }\n"
    "}\n"
    "test \"empty_iter\" {\n"
    "  var sum: i32 = 0;\n"
    "  var it = .RangeIter{.current = 5, .end = 5};\n"
    "  foreach (x of it) {\n"
    "    sum = sum + 1;\n"
    "  }\n"
    "  assert(sum == 0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d) printf("  diag[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_foreach, foreach_break) {
  const char *src = BUILTIN_ASSERT
    "struct IterResult {\n"
    "  value: i32;\n"
    "  done: bool;\n"
    "}\n"
    "struct RangeIter {\n"
    "  current: i32;\n"
    "  end: i32;\n"
    "  func next(self: *RangeIter): IterResult {\n"
    "    if (self.current >= self.end) {\n"
    "      return .IterResult{.value = 0, .done = true};\n"
    "    }\n"
    "    var v = self.current;\n"
    "    self.current = self.current + 1;\n"
    "    return .IterResult{.value = v, .done = false};\n"
    "  }\n"
    "}\n"
    "test \"foreach_break\" {\n"
    "  var sum: i32 = 0;\n"
    "  var it = .RangeIter{.current = 0, .end = 100};\n"
    "  foreach (x of it) {\n"
    "    if (x >= 3) { break; }\n"
    "    sum = sum + x;\n"
    "  }\n"
    "  assert(sum == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d) printf("  diag[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_foreach, foreach_continue) {
  const char *src = BUILTIN_ASSERT
    "struct IterResult {\n"
    "  value: i32;\n"
    "  done: bool;\n"
    "}\n"
    "struct RangeIter {\n"
    "  current: i32;\n"
    "  end: i32;\n"
    "  func next(self: *RangeIter): IterResult {\n"
    "    if (self.current >= self.end) {\n"
    "      return .IterResult{.value = 0, .done = true};\n"
    "    }\n"
    "    var v = self.current;\n"
    "    self.current = self.current + 1;\n"
    "    return .IterResult{.value = v, .done = false};\n"
    "  }\n"
    "}\n"
    "test \"foreach_continue\" {\n"
    "  var sum: i32 = 0;\n"
    "  var it = .RangeIter{.current = 0, .end = 5};\n"
    "  foreach (x of it) {\n"
    "    if (x == 2) { continue; }\n"
    "    sum = sum + x;\n"
    "  }\n"
    "  assert(sum == 8);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d) printf("  diag[%zu]: %s\n", i, d->message);
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== error: foreach on non-iterator ===== */

TEST_F(dt_foreach, foreach_no_next_method) {
  const char *src = BUILTIN_ASSERT
    "struct NotIter {\n"
    "  x: i32;\n"
    "}\n"
    "test \"no_next\" {\n"
    "  var it = .NotIter{.x = 5};\n"
    "  foreach (v of it) {\n"
    "    assert(false);\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
