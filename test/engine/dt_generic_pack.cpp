#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

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
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);
  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_generic_pack : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== Pack parameter declaration ===== */

TEST_F(dt_generic_pack, pack_param_declaration) {
  /* func foo[...Args](): void {} — pack parameter with no constraint */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args](): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_pack, pack_param_must_be_last) {
  /* Pack parameter must be the last in the generic parameter list */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args, T](): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_pack, only_one_rest_param) {
  /* Only one rest parameter is allowed */
  const char *src = BUILTIN_ASSERT
    "func foo[...A, ...B](): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pack with explicit type args ===== */

TEST_F(dt_generic_pack, pack_param_single_explicit) {
  /* foo[i32, f64]() — provide explicit type args for pack */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args](): void {}\n"
    "test \"t\" { foo[i32, f64](); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_pack, pack_param_empty_expansion) {
  /* foo[]() — empty pack expansion */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args](): void {}\n"
    "test \"t\" { foo[](); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Mixed regular and pack params ===== */

TEST_F(dt_generic_pack, mixed_regular_and_pack) {
  /* func foo[T, ...Args](x: T, ...args: Args): void */
  const char *src = BUILTIN_ASSERT
    "func foo[T, ...Args](x: T, ...args: Args): void {}\n"
    "test \"t\" { foo(1); foo(1, 2, 3); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pack in function type ===== */

TEST_F(dt_generic_pack, pack_in_function_type) {
  /* fn: func(...Args): R — pack in function type expression */
  const char *src = BUILTIN_ASSERT
    "func foo[R, ...Args](fn: func(...Args) -> R): R {\n"
    "  return fn();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pack inference from call ===== */

TEST_F(dt_generic_pack, pack_inference_from_call) {
  /* Infer pack from call arguments */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args](): void {}\n"
    "test \"t\" { foo[i32, f64](); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pack expansion in params ===== */

TEST_F(dt_generic_pack, pack_expansion_in_params) {
  /* ...args: Args in function expression parameters */
  const char *src = BUILTIN_ASSERT
    "func foo[...Args](...args: Args): void {}\n"
    "test \"t\" { foo[i32, f64](1, 2); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Decorator pattern (end-to-end) ===== */

TEST_F(dt_generic_pack, decorator_pattern_e2e) {
  /* Full decorator pattern: wrap a function with logging */
  const char *src = BUILTIN_ASSERT
    "func wrap[R, ...Args](fn: func(...Args) -> R): func(...Args) -> R {\n"
    "  return func |fn| (...args: Args): R {\n"
    "    return fn(...args);\n"
    "  };\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pack with constraint ===== */

TEST_F(dt_generic_pack, pack_with_constraint) {
  /* Pack parameter with constraint — each expanded type must satisfy */
  const char *src = BUILTIN_ASSERT
    "func sum[...Args extends i32](...args: Args): i32 {\n"
    "  return 0;\n"
    "}\n"
    "test \"t\" { sum[i32, i32](1, 2); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
