#include "engine/context.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
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

class dt_generic_inference : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Type Inference Tests ===== */

TEST_F(dt_generic_inference, infer_single_i32) {
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"t\" { id[i32](42); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_single_f64) {
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"t\" { id[f64](3.14); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_from_call_arg_i32) {
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"t\" { id(42); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_from_call_arg_f64) {
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"t\" { id(3.14); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_two_params) {
  const char *src = BUILTIN_ASSERT
    "func pair[A, B](a: A, b: B): void {}\n"
    "test \"t\" { pair(1, 2.0); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_same_param_consistency) {
  const char *src = BUILTIN_ASSERT
    "func both[T](a: T, b: T): void {}\n"
    "test \"t\" { both(1, 2); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_pointer_param) {
  const char *src = BUILTIN_ASSERT
    "func deref[T](p: *T): T { return p.*; }\n"
    "test \"t\" {\n"
    "  var x: i32 = 10;\n"
    "  deref(x.&);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_slice_param) {
  const char *src = BUILTIN_ASSERT
    "func first[T](s: []T): T { return s[0]; }\n"
    "test \"t\" {\n"
    "  var arr: [3]i32 = .{0, 0, 0};\n"
    "  first(arr[:]);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_mismatch_error) {
  /* Same T used in two positions with incompatible types should error */
  const char *src = BUILTIN_ASSERT
    "func both[T](a: T, b: T): void {}\n"
    "test \"t\" { both(1, 2.0); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_unresolved_error) {
  /* T only in return position, cannot be inferred from args */
  const char *src = BUILTIN_ASSERT
    "func make[T](): T { return 0; }\n"
    "test \"t\" { make(); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Constraint Checking Tests ===== */

TEST_F(dt_generic_inference, constraint_structural_pass) {
  const char *src = BUILTIN_ASSERT
    "func use[T extends struct { x: i32; }](v: T): void {}\n"
    "struct Vec2 { x: i32; y: i32; }\n"
    "test \"t\" { use(.Vec2 { .x = 1, .y = 2 }); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_structural_fail) {
  const char *src = BUILTIN_ASSERT
    "func use[T extends struct { x: i32; }](v: T): void {}\n"
    "struct NoX { y: i32; z: i32; }\n"
    "test \"t\" { use(.NoX { .y = 1, .z = 2 }); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_generic_instance) {
  /* Container[?] constraint: T must be a generic instance of Container */
  const char *src = BUILTIN_ASSERT
    "struct Container[T] { data: T; }\n"
    "func process[T extends Container[?]](c: T): void {}\n"
    "test \"t\" {\n"
    "  var c = .Container[i32]{.data = 0};\n"
    "  process(c);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_pointer) {
  const char *src = BUILTIN_ASSERT
    "func read[T extends *?](p: T): void {}\n"
    "test \"t\" {\n"
    "  var x: i32 = 1;\n"
    "  read(x.&);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_wildcard_skips) {
  /* Wildcard in constraint allows any type arg at that position */
  const char *src = BUILTIN_ASSERT
    "struct Pair[A, B] { first: A; second: B; }\n"
    "func test_pair[T extends Pair[?, ?]](p: T): void {}\n"
    "test \"t\" {\n"
    "  var p = .Pair[i32, f64]{.first = 1, .second = 2.0};\n"
    "  test_pair(p);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Tuple Constraint <?> Tests ===== */

TEST_F(dt_generic_inference, constraint_tuple_wildcard_pass) {
  /* T extends <?> means T must be a tuple type — <i32, f64> satisfies */
  const char *src = BUILTIN_ASSERT
    "func first[T extends <?>](t: T): void {}\n"
    "test \"t\" {\n"
    "  var tup: <i32, f64> = .<i32, f64>{1, 2.0};\n"
    "  first(tup);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_tuple_wildcard_multi_elem) {
  /* <?> accepts any tuple regardless of element count */
  const char *src = BUILTIN_ASSERT
    "func process[T extends <?>](t: T): void {}\n"
    "test \"t\" {\n"
    "  var t1: <i32, i32, i32> = .<i32, i32, i32>{1, 2, 3};\n"
    "  process(t1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_tuple_wildcard_fail_int) {
  /* T extends <?> should reject non-tuple types like i32 */
  const char *src = BUILTIN_ASSERT
    "func process[T extends <?>](t: T): void {}\n"
    "test \"t\" {\n"
    "  process(42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_tuple_wildcard_fail_struct) {
  /* T extends <?> should reject struct types */
  const char *src = BUILTIN_ASSERT
    "func process[T extends <?>](t: T): void {}\n"
    "struct Point { x: i32; y: i32; }\n"
    "test \"t\" {\n"
    "  process(.Point { .x = 1, .y = 2 });\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_tuple_wildcard_fail_pointer) {
  /* T extends <?> should reject pointer types */
  const char *src = BUILTIN_ASSERT
    "func process[T extends <?>](t: T): void {}\n"
    "test \"t\" {\n"
    "  var x: i32 = 0;\n"
    "  process(x.&);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, constraint_tuple_wildcard_empty_tuple) {
  /* <?> should also accept empty tuple <> */
  const char *src = BUILTIN_ASSERT
    "func process[T extends <?>](t: T): void {}\n"
    "test \"t\" {\n"
    "  var e: <> = .<>{};\n"
    "  process(e);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Inference + Constraint Combination Tests ===== */

TEST_F(dt_generic_inference, infer_with_constraint_pass) {
  const char *src = BUILTIN_ASSERT
    "func id[T extends struct { x: i32; }](v: T): T { return v; }\n"
    "struct Point { x: i32; y: i32; }\n"
    "test \"t\" { id(.Point { .x = 1, .y = 2 }); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_with_constraint_fail) {
  const char *src = BUILTIN_ASSERT
    "func id[T extends struct { x: i32; }](v: T): T { return v; }\n"
    "struct NoX { y: i32; }\n"
    "test \"t\" { id(.NoX { .y = 1 }); }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Multiple Instantiation Tests ===== */

TEST_F(dt_generic_inference, same_function_different_type_args) {
  /* Same generic function instantiated with different type args */
  const char *src = BUILTIN_ASSERT
    "func id[T](x: T): T { return x; }\n"
    "test \"t\" {\n"
    "  var a: i32 = id[i32](42);\n"
    "  var b: f64 = id[f64](3.14);\n"
    "  var c: i32 = id(1);\n"
    "  var d: f64 = id(2.0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, same_function_multiple_inferred_calls) {
  /* Same generic function called multiple times with different inferred types */
  const char *src = BUILTIN_ASSERT
    "func pair[A, B](a: A, b: B): void {}\n"
    "test \"t\" {\n"
    "  pair(1, 2.0);\n"
    "  pair(3.0, 4);\n"
    "  pair(\"hello\", true);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_from_func_return_type) {
  /* U should be inferred from the return type of the fn parameter */
  const char *src = BUILTIN_ASSERT
    "func apply[U](fn: func() -> U): void {}\n"
    "test \"t\" {\n"
    "  apply(func(): i32 { return 42; });\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, infer_from_func_param_type) {
  /* T should be inferred from the parameter type of the fn parameter */
  const char *src = BUILTIN_ASSERT
    "func consume[T](fn: func(T) -> void, val: T): void {}\n"
    "test \"t\" {\n"
    "  consume(func(x: i32): void {}, 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, generic_type_alias) {
  /* type Pair[A,B] = struct { first: A; second: B; }; */
  const char *src =
    "type Pair[A, B] = struct { first: A; second: B; };\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_generic_inference, generic_type_alias_instantiation) {
  /* Instantiate a generic type alias */
  const char *src = BUILTIN_ASSERT
    "type Pair[A, B] = struct { first: A; second: B; };\n"
    "test \"t\" {\n"
    "  var x: Pair[i32, f64] = .Pair[i32, f64]{1, 2.0};\n"
    "  assert(x.first == 1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
