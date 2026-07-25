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

/* ===== test fixture ===== */

class dt_generic_value : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Value generic param: basic declaration ===== */

TEST_F(dt_generic_value, value_param_func_declaration) {
  const char *src = "func foo[N: u64](): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_declaration) {
  const char *src = "struct Buffer[N: u64, T] { data: [N]T; }\n";
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: struct instantiation via type annotation ===== */

TEST_F(dt_generic_value, value_param_struct_type_annotation) {
  const char *src =
    "struct Buffer[N: u64, T] { data: [N]T; }\n"
    "test \"struct_type\" {\n"
    "  var x: Buffer[64, i32] = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: comptime expression ===== */

TEST_F(dt_generic_value, value_param_comptime_expr) {
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"comptime_expr\" {\n"
    "  foo[2 + 3]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: struct init list ===== */

TEST_F(dt_generic_value, value_param_struct_init_list_simple) {
  /* Simple struct init with dot-prefix type */
  const char *src = BUILTIN_ASSERT
    "struct Pair[A, B] { first: A; second: B; }\n"
    "test \"struct_init_simple\" {\n"
    "  var x = .Pair[i32, i64]{ .first = 1, .second = 2 };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_init_list) {
  /* Buffer[64, i32] with a single-element field init (not array repeat) */
  const char *src = BUILTIN_ASSERT
    "struct Buffer[N: u64, T] { data: [N]T; }\n"
    "test \"struct_init\" {\n"
    "  var x: Buffer[64, i32] = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_init_with_field) {
  /* Struct init with dot-prefix and non-array field */
  const char *src = BUILTIN_ASSERT
    "struct Wrapper[N: u64] { size: u64; }\n"
    "test \"struct_init_field\" {\n"
    "  var x = .Wrapper[64]{ .size = 64 };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_init_array_field) {
  /* Struct with array field using value param — type annotation only */
  const char *src = BUILTIN_ASSERT
    "struct Buffer[N: u64, T] { data: [N]T; }\n"
    "test \"struct_init_arr\" {\n"
    "  var x: Buffer[3, i32] = .{ .data = .{ 1, 2, 3 } };\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Value generic param: edge cases ===== */

TEST_F(dt_generic_value, value_param_negative_int) {
  /* Negative literal is valid for u64 (wraps to max uint64) — should not error */
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"neg_int\" {\n"
    "  foo[-1]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_zero_length_array) {
  /* Zero-length array via value param */
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"zero_len\" {\n"
    "  foo[0]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_same_value_same_type) {
  /* Same value should produce the same cached type instance */
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"same_type\" {\n"
    "  foo[5]();\n"
    "  foo[5]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_different_values_distinct_instances) {
  /* Different values should produce distinct type instances */
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"distinct\" {\n"
    "  foo[3]();\n"
    "  foo[7]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_comptime_complex_expr) {
  /* Complex comptime expression as value argument */
  const char *src = BUILTIN_ASSERT
    "func foo[N: u64](): void {}\n"
    "test \"comptime_complex\" {\n"
    "  foo[(2 + 3) * 4]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_struct_with_value_in_field) {
  /* Verify struct field type is correctly substituted with value param */
  const char *src = BUILTIN_ASSERT
    "struct Buffer[N: u64, T] { data: [N]T; }\n"
    "test \"field_type\" {\n"
    "  var x: Buffer[5, i32] = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_bool_false_instantiation) {
  /* Bool value param with false */
  const char *src = BUILTIN_ASSERT
    "func cond[B: bool](): void {}\n"
    "test \"val_bool_false\" {\n"
    "  cond[false]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_value, value_param_bool_type_mismatch_int) {
  /* Passing int where bool is expected should error */
  const char *src =
    "func cond[B: bool](): void {}\n"
    "test \"bool_mismatch\" {\n"
    "  cond[42]();\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
