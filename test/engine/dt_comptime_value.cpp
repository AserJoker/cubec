#include "engine/comptime_value.h"
#include "engine/checker.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_comptime_value : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== nil ===== */

TEST_F(dt_comptime_value, create_nil) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_nil(allocator, ctx->builtin_void);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_NIL);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, nil_not_truthy) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_nil(allocator, ctx->builtin_void);
  EXPECT_FALSE(comptime_value_is_truthy(v));
  checker_dispose(ctx);
}

/* ===== bool ===== */

TEST_F(dt_comptime_value, create_bool) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t t = comptime_value_create_bool(allocator, true, ctx->builtin_bool);
  comptime_value_t f = comptime_value_create_bool(allocator, false, ctx->builtin_bool);
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(t->bool_val);
  EXPECT_TRUE(comptime_value_is_truthy(t));
  EXPECT_FALSE(comptime_value_is_truthy(f));
  checker_dispose(ctx);
}

/* ===== int ===== */

TEST_F(dt_comptime_value, create_int_signed) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, -42, 0, 32, true,
                                                   ctx->builtin_i32);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, -42);
  EXPECT_TRUE(v->int_val.is_signed);
  EXPECT_EQ(v->int_val.width, 32);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, create_int_unsigned) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, 0, 300, 16, false,
                                                   ctx->builtin_u16);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.u, 300u);
  EXPECT_FALSE(v->int_val.is_signed);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, int_truthy) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t zero = comptime_value_create_int(allocator, 0, 0, 32, true,
                                                      ctx->builtin_i32);
  comptime_value_t one = comptime_value_create_int(allocator, 1, 1, 32, true,
                                                     ctx->builtin_i32);
  EXPECT_FALSE(comptime_value_is_truthy(zero));
  EXPECT_TRUE(comptime_value_is_truthy(one));
  checker_dispose(ctx);
}

/* ===== float ===== */

TEST_F(dt_comptime_value, create_float) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_float(allocator, 3.14, 64,
                                                     ctx->builtin_f64);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_FLOAT);
  EXPECT_DOUBLE_EQ(v->float_val.value, 3.14);
  EXPECT_EQ(v->float_val.width, 64);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, float_truthy) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t zero = comptime_value_create_float(allocator, 0.0, 64,
                                                        ctx->builtin_f64);
  comptime_value_t nonzero = comptime_value_create_float(allocator, 1.5, 64,
                                                          ctx->builtin_f64);
  EXPECT_FALSE(comptime_value_is_truthy(zero));
  EXPECT_TRUE(comptime_value_is_truthy(nonzero));
  checker_dispose(ctx);
}

/* ===== char ===== */

TEST_F(dt_comptime_value, create_char) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_char(allocator, 'A', ctx->builtin_char);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_CHAR);
  EXPECT_EQ(v->char_val, 'A');
  checker_dispose(ctx);
}

/* ===== string ===== */

TEST_F(dt_comptime_value, create_string) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_string(allocator, "hello",
                                                      ctx->builtin_string);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  EXPECT_TRUE(comptime_value_is_truthy(v));
  checker_dispose(ctx);
}

/* ===== type ===== */

TEST_F(dt_comptime_value, create_type) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_type(allocator, ctx->builtin_i32);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_TYPE);
  EXPECT_EQ(v->type_val, ctx->builtin_i32);
  checker_dispose(ctx);
}

/* ===== pointer ===== */

TEST_F(dt_comptime_value, create_pointer) {
  checker_t ctx = checker_create(allocator);
  semantic_type_t ptr_type = NULL; /* simplified — real ptr types need resolver */
  comptime_value_t v = comptime_value_create_pointer(allocator, 42, ptr_type);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_POINTER);
  EXPECT_EQ(v->pointer.addr, 42u);
  checker_dispose(ctx);
}

/* ===== error ===== */

TEST_F(dt_comptime_value, create_error) {
  comptime_value_t v = comptime_value_create_error(allocator);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_ERROR);
  EXPECT_FALSE(comptime_value_is_truthy(v));
}

/* ===== equals ===== */

TEST_F(dt_comptime_value, equals_bool) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t a = comptime_value_create_bool(allocator, true, ctx->builtin_bool);
  comptime_value_t b = comptime_value_create_bool(allocator, true, ctx->builtin_bool);
  comptime_value_t c = comptime_value_create_bool(allocator, false, ctx->builtin_bool);
  EXPECT_TRUE(comptime_value_equals(a, b));
  EXPECT_FALSE(comptime_value_equals(a, c));
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, equals_int) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t a = comptime_value_create_int(allocator, 42, 42, 32, true,
                                                   ctx->builtin_i32);
  comptime_value_t b = comptime_value_create_int(allocator, 42, 42, 32, true,
                                                   ctx->builtin_i32);
  EXPECT_TRUE(comptime_value_equals(a, b));
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, equals_nil) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t a = comptime_value_create_nil(allocator, ctx->builtin_void);
  comptime_value_t b = comptime_value_create_nil(allocator, ctx->builtin_void);
  EXPECT_TRUE(comptime_value_equals(a, b));
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, not_equal_different_kinds) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t a = comptime_value_create_int(allocator, 1, 1, 32, true,
                                                   ctx->builtin_i32);
  comptime_value_t b = comptime_value_create_bool(allocator, true, ctx->builtin_bool);
  EXPECT_FALSE(comptime_value_equals(a, b));
  checker_dispose(ctx);
}

/* ===== clone ===== */

TEST_F(dt_comptime_value, clone_int) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, -99, 0, 32, true,
                                                   ctx->builtin_i32);
  comptime_value_t c = comptime_value_clone(allocator, v);
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(c->int_val.s, -99);
  EXPECT_NE(v, c); /* different allocation */
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, clone_string) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_string(allocator, "abc",
                                                      ctx->builtin_string);
  comptime_value_t c = comptime_value_clone(allocator, v);
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(c), "abc");
  checker_dispose(ctx);
}

/* ===== conversions ===== */

TEST_F(dt_comptime_value, as_i64) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, -42, 0, 32, true,
                                                   ctx->builtin_i32);
  EXPECT_EQ(comptime_value_as_i64(v), -42);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, as_u64) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, 0, 300, 16, false,
                                                   ctx->builtin_u16);
  EXPECT_EQ(comptime_value_as_u64(v), 300u);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, as_f64_from_int) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, 3, 3, 32, true,
                                                   ctx->builtin_i32);
  EXPECT_DOUBLE_EQ(comptime_value_as_f64(v), 3.0);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_value, as_f64_from_float) {
  checker_t ctx = checker_create(allocator);
  comptime_value_t v = comptime_value_create_float(allocator, 2.5, 64,
                                                     ctx->builtin_f64);
  EXPECT_DOUBLE_EQ(comptime_value_as_f64(v), 2.5);
  checker_dispose(ctx);
}
