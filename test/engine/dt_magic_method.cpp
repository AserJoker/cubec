#include "engine/context.h"
#include "engine/builtin.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_CAST "builtin func cast[T,K](expr:K):T;\n"

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

class dt_magic_method : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== __get__ ===== */

TEST_F(dt_magic_method, get_type_check) {
  /* struct with __get__ supports obj[key] */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Vec { data: i32; func __get__(self:*Vec, idx:i32):i32 { return self.data; } }\n"
    "test \"get_type\" {\n"
    "  var v: Vec = .Vec{.data = 42};\n"
    "  var x: i32 = v[0];\n"
    "  assert(x == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, get_comptime_eval) {
  /* comptime: obj[key] actually calls __get__ */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Vec { data: i32; func __get__(self:*Vec, idx:i32):i32 { return self.data; } }\n"
    "test \"get_comptime\" {\n"
    "  var v: Vec = .Vec{.data = 7};\n"
    "  assert(v[0] == 7);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, get_missing_error) {
  /* No __get__ → obj[key] is an error */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; }\n"
    "test \"no_get\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  var y: i32 = s[0];\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== __set__ ===== */

TEST_F(dt_magic_method, set_type_check) {
  /* struct with __set__ supports obj[key] = value */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Vec { data: i32; func __set__(self:*Vec, idx:i32, val:i32):void { self.data = val; } }\n"
    "test \"set_type\" {\n"
    "  var v: Vec = .Vec{.data = 0};\n"
    "  v[0] = 99;\n"
    "  assert(v.data == 99);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, set_comptime_eval) {
  /* comptime: obj[key] = value calls __set__ */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Vec { data: i32; func __set__(self:*Vec, idx:i32, val:i32):void { self.data = val; } }\n"
    "test \"set_comptime\" {\n"
    "  var v: Vec = .Vec{.data = 0};\n"
    "  v[0] = 42;\n"
    "  assert(v.data == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, set_missing_error) {
  /* No __set__ → obj[key] = value is an error */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; }\n"
    "test \"no_set\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  s[0] = 99;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== __call__ ===== */

TEST_F(dt_magic_method, call_type_check) {
  /* struct with __call__ supports obj(args) */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Adder { offset: i32; func __call__(self:*Adder, x:i32):i32 { return self.offset + x; } }\n"
    "test \"call_type\" {\n"
    "  var a: Adder = .Adder{.offset = 10};\n"
    "  var r: i32 = a(5);\n"
    "  assert(r == 15);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, call_comptime_eval) {
  /* comptime: obj(args) actually calls __call__ */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Adder { offset: i32; func __call__(self:*Adder, x:i32):i32 { return self.offset + x; } }\n"
    "test \"call_comptime\" {\n"
    "  var a: Adder = .Adder{.offset = 10};\n"
    "  assert(a(5) == 15);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, call_missing_error) {
  /* No __call__ → obj(args) is an error */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; }\n"
    "test \"no_call\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  var r: i32 = s(42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, call_pointer_no_magic) {
  /* Pointer types don't have __call__, so ptr() is an error */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; func __call__(self:*S, v:i32):i32 { return v; } }\n"
    "test \"ptr_call\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  var p: *S = s.&;\n"
    "  var r: i32 = p(42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== __slice__ ===== */

TEST_F(dt_magic_method, slice_type_check) {
  /* struct with __slice__ supports obj[start:len] */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Buf { data: i32; func __slice__(self:*Buf, start:i32, len:i32):i32 { return self.data; } }\n"
    "test \"slice_type\" {\n"
    "  var b: Buf = .Buf{.data = 1};\n"
    "  var s: i32 = b[0:1];\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, slice_comptime_eval) {
  /* comptime: obj[start:len] calls __slice__ */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Buf { data: i32; func __slice__(self:*Buf, start:i32, len:i32):i32 { return self.data; } }\n"
    "test \"slice_comptime\" {\n"
    "  var b: Buf = .Buf{.data = 42};\n"
    "  var s: i32 = b[0:1];\n"
    "  assert(s == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, slice_missing_error) {
  /* No __slice__ → obj[start:len] is an error for non-array/slice types */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; }\n"
    "test \"no_slice\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  var r: []i32 = s[0:1];\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

/* ===== __value__ ===== */

TEST_F(dt_magic_method, value_binary_op) {
  /* obj + 1 auto-calls __value__ for arithmetic */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Wrapped { val: i32; func __value__(self:*Wrapped):i32 { return self.val; } }\n"
    "test \"value_binary\" {\n"
    "  var w: Wrapped = .Wrapped{.val = 10};\n"
    "  var r: i32 = w + 5;\n"
    "  assert(r == 15);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, value_comparison) {
  /* obj == 10 auto-calls __value__ for comparison */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Wrapped { val: i32; func __value__(self:*Wrapped):i32 { return self.val; } }\n"
    "test \"value_cmp\" {\n"
    "  var w: Wrapped = .Wrapped{.val = 10};\n"
    "  assert(w == 10);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, value_implicit_convert) {
  /* var v: i32 = myStruct uses __value__ for implicit conversion */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Wrapped { val: i32; func __value__(self:*Wrapped):i32 { return self.val; } }\n"
    "test \"value_implicit\" {\n"
    "  var w: Wrapped = .Wrapped{.val = 42};\n"
    "  var x: i32 = w;\n"
    "  assert(x == 42);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, value_not_called_when_unnecessary) {
  /* var w: Wrapped = .Wrapped{} should NOT call __value__ */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Wrapped { val: i32; func __value__(self:*Wrapped):i32 { return self.val; } }\n"
    "test \"value_unnecessary\" {\n"
    "  var w: Wrapped = .Wrapped{.val = 5};\n"
    "  assert(w.val == 5);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(r.ctx->test_fail_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, value_missing_error) {
  /* No __value__ → arithmetic with struct is an error */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct S { x: i32; }\n"
    "test \"no_value\" {\n"
    "  var s: S = .S{.x = 1};\n"
    "  var r: i32 = s + 1;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0u);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_magic_method, value_bitwise_op) {
  /* obj & mask auto-calls __value__ for bitwise */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Flags { bits: i32; func __value__(self:*Flags):i32 { return self.bits; } }\n"
    "test \"value_bitwise\" {\n"
    "  var f: Flags = .Flags{.bits = 0xFF};\n"
    "  var r: i32 = f & 0x0F;\n"
    "  assert(r == 0x0F);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
