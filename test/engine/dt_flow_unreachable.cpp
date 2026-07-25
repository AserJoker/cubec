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

static size_t count_warnings(context_t ctx) {
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
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== tests ===== */

TEST_F(dt_flow_unreachable, unreachable_after_return) {
  const char *src = BUILTIN_ASSERT
    "func foo(): void {\n"
    "  return;\n"
    "  var x = 5;\n"
    "}\n";
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  EXPECT_EQ(count_warnings(r.ctx), 1u);
  compile_result_cleanup(&r, allocator);
}
