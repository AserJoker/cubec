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

/* ===== fixture ===== */

class dt_flow_return : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== tests ===== */

TEST_F(dt_flow_return, void_function_no_return_ok) {
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  var x = 5;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_return, non_void_missing_return_error) {
  const char *src = BUILTIN_ASSERT
    "func foo(): i32 {\n"
    "  var x = 5;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_return, non_void_all_paths_return_ok) {
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): i32 {\n"
    "  if (cond) {\n"
    "    return 1;\n"
    "  } else {\n"
    "    return 2;\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_return, non_void_one_path_missing_return) {
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): i32 {\n"
    "  if (cond) {\n"
    "    return 1;\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_return, loop_return_may_not_execute) {
  /* Return inside loop doesn't guarantee execution */
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): i32 {\n"
    "  while (cond) {\n"
    "    return 1;\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_return, non_void_explicit_return_ok) {
  const char *src = BUILTIN_ASSERT
    "func foo(): i32 {\n"
    "  return 42;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
