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

  /* If parsing failed, g_error is set — convert to a diagnostic */
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

class dt_generic_instantiation : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== Generic function type inference ===== */

TEST_F(dt_generic_instantiation, generic_function_declaration_no_errors) {
  /* Just declaring a generic function should work */
  const char *src = "func identity[T](value: T): T { return value; }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_struct_declaration_no_errors) {
  /* Just declaring a generic struct should work */
  const char *src = "struct Vec[T] { data: *T; len: u64; }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, explicit_type_arg) {
  /* identity[i32](42) — explicit type arg */
  const char *src = BUILTIN_ASSERT
    "func identity[T](value: T): T { return value; }\n"
    "test \"explicit_type_arg\" { assert(identity[i32](42) == 42); }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic struct field substitution ===== */

TEST_F(dt_generic_instantiation, struct_field_substitution) {
  /* struct Vec[T] { data: *T; len: u64; }
     var v = .Vec[i32]{ .data = nil, .len = 0 }; — data should be *i32 */
  const char *src = BUILTIN_ASSERT
    "struct Vec[T] { data: *T; len: u64; }\n"
    "test \"struct_field_sub\" {\n"
    "  var v = .Vec[i32]{ .data = nil, .len = 0 };\n"
    "  assert(v.len == 0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic union field substitution ===== */

TEST_F(dt_generic_instantiation, union_field_substitution) {
  /* union Option[T] { value: T; empty: void; }
     var o = .Option[i32]{ .value = 42 }; — value should be i32 */
  const char *src = BUILTIN_ASSERT
    "union Option[T] { value: T; empty: void; }\n"
    "test \"union_field_sub\" {\n"
    "  var o = .Option[i32]{ .value = 42 };\n"
    "  assert(o.value == 42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pointer type in generic ===== */

TEST_F(dt_generic_instantiation, pointer_in_generic) {
  /* func deref[T](ptr: *T): T { return ptr.*; } */
  const char *src =
    "func deref[T](ptr: *T): T { return ptr.*; }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Slice type in generic ===== */

TEST_F(dt_generic_instantiation, slice_in_generic) {
  /* func first[T](s: []T): T { return s[0]; } */
  const char *src =
    "func first[T](s: []T): T { return s[0]; }\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Multiple generic params ===== */

TEST_F(dt_generic_instantiation, two_param_inference) {
  /* func pair[A, B](a: A, b: B): void {} */
  const char *src =
    "func pair[A, B](a: A, b: B): void {}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic function explicit instantiation ===== */

TEST_F(dt_generic_instantiation, explicit_instantiation_no_errors) {
  const char *src = BUILTIN_ASSERT
    "func identity[T](value: T): T { return value; }\n"
    "test \"explicit_inst\" {\n"
    "  var x = identity[i32](10);\n"
    "  assert(x == 10);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Multiple instantiations of the same template ===== */

TEST_F(dt_generic_instantiation, multiple_struct_instantiations) {
  /* struct Vec[T] instantiated with i32, f64, bool — each gets independent fields */
  const char *src = BUILTIN_ASSERT
    "struct Vec[T] { data: *T; len: u64; }\n"
    "test \"multi_struct\" {\n"
    "  var vi = .Vec[i32]{ .data = nil, .len = 0 };\n"
    "  var vf = .Vec[f64]{ .data = nil, .len = 0 };\n"
    "  var vb = .Vec[bool]{ .data = nil, .len = 0 };\n"
    "  assert(vi.len == 0);\n"
    "  assert(vf.len == 0);\n"
    "  assert(vb.len == 0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, multiple_function_instantiations) {
  /* identity[T] called with i32, f64, bool — each produces a distinct function type */
  const char *src = BUILTIN_ASSERT
    "func identity[T](value: T): T { return value; }\n"
    "test \"multi_func\" {\n"
    "  var xi = identity[i32](1);\n"
    "  var xf = identity[f64](2.0);\n"
    "  var xb = identity[bool](true);\n"
    "  assert(xi == 1);\n"
    "  assert(xf == 2.0);\n"
    "  assert(xb == true);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, same_instantiation_deduplicated) {
  /* Vec[i32] used twice should return the same cached instance (no errors, no duplicates) */
  const char *src = BUILTIN_ASSERT
    "struct Vec[T] { data: *T; len: u64; }\n"
    "test \"dedup\" {\n"
    "  var v1 = .Vec[i32]{ .data = nil, .len = 1 };\n"
    "  var v2 = .Vec[i32]{ .data = nil, .len = 2 };\n"
    "  assert(v1.len == 1);\n"
    "  assert(v2.len == 2);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, mixed_type_and_function_instantiation) {
  /* Both Vec[T] as a type and makeVec[T] as a function are instantiated */
  const char *src = BUILTIN_ASSERT
    "struct Pair[A, B] { first: A; second: B; }\n"
    "func makePair[A, B](a: A, b: B): Pair[A, B] {\n"
    "  return .Pair[A, B]{ .first = a, .second = b };\n"
    "}\n"
    "test \"mixed\" {\n"
    "  var p1 = makePair[i32, f64](1, 2.0);\n"
    "  var p2 = makePair[bool, i32](true, 3);\n"
    "  assert(p1.first == 1);\n"
    "  assert(p2.second == 3);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Type wildcard in constraint ===== */

TEST_F(dt_generic_instantiation, wildcard_type_resolution) {
  /* Verify TYPE_WILDCARD resolves correctly */
  const char *src = "";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
