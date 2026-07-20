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

static size_t count_warnings(checker_t ctx) {
  size_t count = 0;
  size_t total = diagnostic_list_get_size(ctx->diagnostics);
  for (size_t i = 0; i < total; i++) {
    struct diagnostic *d = diagnostic_list_get(ctx->diagnostics, i);
    if (d && d->severity == DIAGNOSTIC_WARNING) count++;
  }
  return count;
}

/* ===== fixture ===== */

class dt_flow_unreachable : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== tests ===== */

TEST_F(dt_flow_unreachable, unreachable_after_return) {
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  return;\n"
    "  var x = 5;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  EXPECT_GE(count_warnings(r.ctx), 1u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_unreachable, unreachable_after_break) {
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  while (true) {\n"
    "    break;\n"
    "    var x = 5;\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  EXPECT_GE(count_warnings(r.ctx), 1u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_unreachable, unreachable_after_continue) {
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  while (true) {\n"
    "    continue;\n"
    "    var x = 5;\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  EXPECT_GE(count_warnings(r.ctx), 1u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_unreachable, reachable_after_if_return) {
  /* Code after if(return) should still be reachable (no else) */
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): void {\n"
    "  if (cond) { return; }\n"
    "  var x = 5;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_unreachable, only_first_unreachable_warned) {
  /* Multiple unreachable statements: only the first should get a warning */
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  return;\n"
    "  var x = 5;\n"
    "  var y = 10;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  EXPECT_EQ(count_warnings(r.ctx), 1u);
  compile_result_cleanup(&r, allocator);
}
