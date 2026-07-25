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

  /* If parsing failed, still create a checker context so error_count is accessible */
  if (!prog || !tokens) {
    ctx->error_count = 1; /* parsing error counts */
    return (struct compile_result){ctx, prog, tokens};
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

class dt_modifiers : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== extern ===== */

TEST_F(dt_modifiers, extern_func) {
  const char *src =
    "extern func malloc(size: u64): *void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, extern_var) {
  const char *src =
    "extern var errno: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== export ===== */

TEST_F(dt_modifiers, export_func) {
  const char *src =
    "export func hello(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== function modifier mutual exclusivity ===== */

TEST_F(dt_modifiers, func_inline_builtin_exclusive) {
  /* inline + builtin: mutually exclusive (builtin has no body, inline requires one) */
  const char *src =
    "inline builtin func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "inline + builtin should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, func_inline_extern_exclusive) {
  /* inline + extern: mutually exclusive */
  const char *src =
    "inline extern func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "inline + extern should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, func_comptime_builtin_exclusive) {
  /* comptime + builtin: mutually exclusive */
  const char *src =
    "comptime builtin func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "comptime + builtin should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, func_comptime_extern_exclusive) {
  /* comptime + extern: mutually exclusive */
  const char *src =
    "comptime extern func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "comptime + extern should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, func_inline_comptime_ignores_inline) {
  /* inline + comptime: comptime takes precedence, inline silently ignored */
  const char *src =
    "inline comptime func foo(): i32 { return 1; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "inline + comptime should be allowed (inline ignored)";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, func_builtin_extern_exclusive) {
  /* builtin + extern: mutually exclusive */
  const char *src =
    "builtin extern func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "builtin + extern should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

/* ===== inline semantics ===== */

TEST_F(dt_modifiers, inline_func_ok) {
  const char *src =
    "inline func add(a: i32, b: i32): i32 { return a + b; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, inline_func_requires_body) {
  /* inline without body: implicitly caught via builtin/extern exclusivity
     (inline + no body is not syntactically distinct from inline+extern) */
  const char *src =
    "inline func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "inline func without body should error";
  compile_result_cleanup(&r, allocator);
}

/* ===== var modifier mutual exclusivity ===== */

TEST_F(dt_modifiers, var_extern_builtin_exclusive) {
  const char *src =
    "extern builtin var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "extern + builtin should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, var_extern_comptime_exclusive) {
  const char *src =
    "extern comptime var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "extern + comptime should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, var_builtin_comptime_exclusive) {
  const char *src =
    "builtin comptime var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "builtin + comptime should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, var_using_extern_exclusive) {
  const char *src =
    "using extern var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "using + extern should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, var_using_builtin_exclusive) {
  const char *src =
    "using builtin var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "using + builtin should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_modifiers, var_using_comptime_exclusive) {
  const char *src =
    "using comptime var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0) << "using + comptime should be mutually exclusive";
  compile_result_cleanup(&r, allocator);
}
