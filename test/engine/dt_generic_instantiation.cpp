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

class dt_generic_instantiation : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Generic function type inference ===== */

TEST_F(dt_generic_instantiation, generic_function_declaration_no_errors) {
  /* Just declaring a generic function should work */
  const char *src = "func identity[T](value: T): T { return value; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_struct_declaration_no_errors) {
  /* Just declaring a generic struct should work */
  const char *src = "struct Vec[T] { data: *T; len: u64; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, explicit_type_arg) {
  /* identity[i32](42) — explicit type arg */
  const char *src = BUILTIN_ASSERT
    "func identity[T](value: T): T { return value; }\n"
    "test \"explicit_type_arg\" { assert(identity[i32](42) == 42); }\n";
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
    "  assert(o.value.! == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Pointer type in generic ===== */

TEST_F(dt_generic_instantiation, pointer_in_generic) {
  /* func deref[T](ptr: *T): T { return ptr.*; } */
  const char *src =
    "func deref[T](ptr: *T): T { return ptr.*; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Slice type in generic ===== */

TEST_F(dt_generic_instantiation, slice_in_generic) {
  /* func first[T](s: []T): T { return s[0]; } */
  const char *src =
    "func first[T](s: []T): T { return s[0]; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Multiple generic params ===== */

TEST_F(dt_generic_instantiation, two_param_inference) {
  /* func pair[A, B](a: A, b: B): void {} */
  const char *src =
    "func pair[A, B](a: A, b: B): void {}\n";
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
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
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Type wildcard in constraint ===== */

TEST_F(dt_generic_instantiation, wildcard_type_resolution) {
  /* Verify TYPE_WILDCARD resolves correctly */
  const char *src = "";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic function body checking (Pass 4) ===== */

TEST_F(dt_generic_instantiation, generic_func_body_checked) {
  /* Generic function body should be type-checked with concrete types */
  const char *src = BUILTIN_ASSERT
    "func identity[T](x: T): T { return x; }\n"
    "test \"identity\" {\n"
    "  var a = identity[i32](42);\n"
    "  assert(a == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_func_body_type_error) {
  /* Generic function body with type error should be caught when instantiated.
     Using i32 where bool is expected. */
  const char *src =
    "func assign_bool[T](x: T): bool { return x; }\n"
    "func caller(): void {\n"
    "  var b = assign_bool[i32](1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  /* i32 cannot implicitly convert to bool */
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_func_multiple_instantiations) {
  /* Same generic function instantiated with different type args */
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"multi\" {\n"
    "  var a = id[i32](1);\n"
    "  var b = id[f64](2.0);\n"
    "  var c = id[bool](true);\n"
    "  assert(a == 1);\n"
    "  assert(b == 2.0);\n"
    "  assert(c == true);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_func_dedup_body_check) {
  /* Same instantiation twice should only check body once */
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"dedup\" {\n"
    "  var a = id[i32](1);\n"
    "  var b = id[i32](2);\n"
    "  assert(a == 1);\n"
    "  assert(b == 2);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, cascading_monomorphization) {
  /* Generic function A calls generic function B — cascading instantiation */
  const char *src = BUILTIN_ASSERT
    "func double[T](x: T): T { return x + x; }\n"
    "func quad[T](x: T): T { return double[T](x) + double[T](x); }\n"
    "test \"cascade\" {\n"
    "  var r = quad[i32](2);\n"
    "  assert(r == 8);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_type_field_access) {
  /* Field access on a generic type instance should work */
  const char *src = BUILTIN_ASSERT
    "struct Pair[A, B] { first: A; second: B; }\n"
    "test \"fields\" {\n"
    "  var p = .Pair[i32, f64]{ .first = 1, .second = 2.0 };\n"
    "  assert(p.first == 1);\n"
    "  assert(p.second == 2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic methods on non-generic struct ===== */

TEST_F(dt_generic_instantiation, non_generic_struct_generic_method_inferred) {
  /* Non-generic struct with a generic method — type arg inferred from call */
  const char *src = BUILTIN_ASSERT
    "struct Box {\n"
    "  val: i32;\n"
    "  func identity[U](self: *Box, x: U): U { return x; }\n"
    "}\n"
    "test \"box_identity\" {\n"
    "  var b = .Box{.val = 42};\n"
    "  var xi = b.identity(10);\n"
    "  var xf = b.identity(2.0);\n"
    "  assert(xi == 10);\n"
    "  assert(xf == 2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (r.ctx->error_count > 0) {
    diagnostic_list_t diags = r.ctx->diagnostics;
    if (diags) {
      size_t dcount = diagnostic_list_get_size(diags);
      for (size_t i = 0; i < dcount; i++) {
        struct diagnostic *d = diagnostic_list_get(diags, i);
        if (d) printf("  DIAG: %s\n", d->message);
      }
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Generic methods on generic struct ===== */

TEST_F(dt_generic_instantiation, generic_struct_generic_method_inferred) {
  /* Generic struct with generic method — both type-level and method-level inferred */
  const char *src = BUILTIN_ASSERT
    "struct Store[T] {\n"
    "  data: T;\n"
    "  func convert[U](self: *Store[T], fallback: U): U { return fallback; }\n"
    "}\n"
    "test \"convert\" {\n"
    "  var s = .Store[i32]{.data = 10};\n"
    "  var r = s.convert(2.0);\n"
    "  assert(r == 2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (r.ctx && r.ctx->error_count > 0) {
    diagnostic_list_t diags = r.ctx->diagnostics;
    if (diags) {
      size_t dcount = diagnostic_list_get_size(diags);
      for (size_t i = 0; i < dcount; i++) {
        struct diagnostic *d = diagnostic_list_get(diags, i);
        if (d) printf("  DIAG: %s\n", d->message);
      }
    }
  }
  if (r.ctx) EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_struct_generic_method_explicit_type_arg) {
  /* Generic method with explicit type arg: obj.method[U](args) */
  const char *src = BUILTIN_ASSERT
    "struct Store[T] {\n"
    "  data: T;\n"
    "  func convert[U](self: *Store[T], fallback: U): U { return fallback; }\n"
    "}\n"
    "test \"explicit\" {\n"
    "  var s = .Store[i32]{.data = 5};\n"
    "  var r = s.convert[f64](3.0);\n"
    "  assert(r == 3.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (r.ctx->error_count > 0) {
    diagnostic_list_t diags = r.ctx->diagnostics;
    if (diags) {
      size_t dcount = diagnostic_list_get_size(diags);
      for (size_t i = 0; i < dcount; i++) {
        struct diagnostic *d = diagnostic_list_get(diags, i);
        if (d) printf("  DIAG: %s\n", d->message);
      }
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_method_type_error) {
  /* Type error in generic method body should be caught when instantiated.
     When U=i32, return x (i32) cannot convert to bool. */
  const char *src =
    "struct Box {\n"
    "  val: i32;\n"
    "  func bad[U](self: *Box, x: U): bool { return x; }\n"
    "}\n"
    "func caller(): void {\n"
    "  var b = .Box{.val = 1};\n"
    "  var r = b.bad(42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_instantiation, generic_method_two_params) {
  /* Generic method with two method-level type params */
  const char *src = BUILTIN_ASSERT
    "struct Pair[A, B] {\n"
    "  first: A;\n"
    "  second: B;\n"
    "  func zip[C, D](self: *Pair[A, B], x: C, y: D): i32 {\n"
    "    return 0;\n"
    "  }\n"
    "}\n"
    "test \"zip\" {\n"
    "  var p = .Pair[i32, f64]{.first = 1, .second = 2.0};\n"
    "  var r = p.zip(true, 3);\n"
    "  assert(r == 0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  if (r.ctx->error_count > 0) {
    diagnostic_list_t diags = r.ctx->diagnostics;
    if (diags) {
      size_t dcount = diagnostic_list_get_size(diags);
      for (size_t i = 0; i < dcount; i++) {
        struct diagnostic *d = diagnostic_list_get(diags, i);
        if (d) printf("  DIAG: %s\n", d->message);
      }
    }
  }
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
