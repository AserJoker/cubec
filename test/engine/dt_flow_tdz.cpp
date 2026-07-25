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

/* ===== fixture ===== */

class dt_flow_tdz : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== tests ===== */

TEST_F(dt_flow_tdz, undefined_use_tdz_error) {
  /* var x: i32 = undefined; then use x in a function body => error */
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  var x: i32 = undefined;\n"
    "  x == 0;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, assign_removes_tdz) {
  /* var x: i32 = undefined; x = 5; x == 5; => OK */
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  var x: i32 = undefined;\n"
    "  x = 5;\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, if_branch_tdz_merge) {
  /* var x: i32 = undefined; if (cond) { x = 5; } x == 5; => error
   * because x may not be assigned if cond is false */
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): void {\n"
    "  var x: i32 = undefined;\n"
    "  if (cond) {\n"
    "    x = 5;\n"
    "  }\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, if_else_both_assign) {
  /* Both branches assign x => not TDZ after merge */
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): void {\n"
    "  var x: i32 = undefined;\n"
    "  if (cond) {\n"
    "    x = 5;\n"
    "  } else {\n"
    "    x = 10;\n"
    "  }\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, loop_body_assign_no_effect) {
  /* Assignment in loop body doesn't guarantee x is assigned after loop */
  const char *src = BUILTIN_ASSERT
    "func foo(cond: bool): void {\n"
    "  var x: i32 = undefined;\n"
    "  while (cond) {\n"
    "    x = 5;\n"
    "  }\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, nested_if_tdz) {
  /* Nested if: x assigned in inner block but not outer */
  const char *src = BUILTIN_ASSERT
    "func foo(a: bool, b: bool): void {\n"
    "  var x: i32 = undefined;\n"
    "  if (a) {\n"
    "    if (b) {\n"
    "      x = 5;\n"
    "    }\n"
    "  }\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GE(r.ctx->error_count, 1);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_flow_tdz, normal_init_no_tdz) {
  /* Normal initializer: var x = 5 => not TDZ, use is fine */
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  var x = 5;\n"
    "  x == 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
