#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
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

  if (g_error) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         (location_t){0}, "%s", g_error->message);
    ctx->error_count++;
    error_clear();
  }

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

/* ===== test fixture ===== */

class dt_generic_value : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== Value generic param: basic declaration ===== */

TEST_F(dt_generic_value, value_param_func_declaration) {
  const char *src = "func foo[N: u64](): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_declaration) {
  const char *src = "struct Buffer[N: u64, T] { data: [N]T; }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: function instantiation ===== */

TEST_F(dt_generic_value, value_param_int_instantiation) {
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"val_int\" {\n"
    "  foo[5]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_different_values_distinct_types) {
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"distinct\" {\n"
    "  foo[5]();\n"
    "  foo[10]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: struct instantiation via type annotation ===== */

TEST_F(dt_generic_value, value_param_struct_type_annotation) {
  const char *src =
    "struct Buffer[N: u64, T] { data: [N]T; }\n"
    "test \"struct_type\" {\n"
    "  var x: Buffer[64, i32];\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: bool value ===== */

TEST_F(dt_generic_value, value_param_bool) {
  const char *src = BUILTIN_ASSERT
    "func cond[B: bool](): void {}\n"
    "test \"val_bool\" {\n"
    "  cond[true]();\n"
    "  cond[false]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: type mismatch error ===== */

TEST_F(dt_generic_value, value_param_type_mismatch) {
  /* func foo[N: u64](): void — foo[true]() should error (bool not compatible with u64) */
  const char *src =
    "func foo[N: u64](): void {}\n"
    "test \"mismatch\" {\n"
    "  foo[true]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: non-constant expression error ===== */

TEST_F(dt_generic_value, value_param_not_constant) {
  const char *src =
    "func foo[N: u64](): void {}\n"
    "test \"not_const\" {\n"
    "  var n = 5;\n"
    "  foo[n]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Same value param produces same cached type ===== */

TEST_F(dt_generic_value, same_value_cached) {
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"cached\" {\n"
    "  foo[5]();\n"
    "  foo[5]();\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
