#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_integer_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_i64_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i64_type(vm));
  }
  type_t _get_const_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_const_i32_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
};

/* ---- i32 create ---- */

TEST_F(it_integer_type, i32_create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_i32_value(vm, 42);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_TRUE(value_is_initialized(v));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i64_create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_i64_value(vm, -10000000000LL);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(v), -10000000000LL);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 equal ---- */

TEST_F(it_integer_type, i32_equal_same) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_equal_different) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 20);
  value_t result = value_equal(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_equal_i64_same_value) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i64_value(vm, 10);
  value_t result = value_equal(vm, a, b);

  /* integer promotion: i32 and i64 → compare as i64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_equal_shadow) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, i32t, NULL, true);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_equal(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 arithmetic ---- */

TEST_F(it_integer_type, i32_add) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 20);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 30);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_sub) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 50);
  value_t b = create_i32_value(vm, 20);
  value_t result = value_sub(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 30);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_mul) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 6);
  value_t b = create_i32_value(vm, 7);
  value_t result = value_mul(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_div) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 100);
  value_t b = create_i32_value(vm, 3);
  value_t result = value_div(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 33);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_div_by_zero_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 100);
  value_t b = create_i32_value(vm, 0);
  value_t result = value_div(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_mod) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 100);
  value_t b = create_i32_value(vm, 3);
  value_t result = value_mod(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 1);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_mod_by_zero_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 100);
  value_t b = create_i32_value(vm, 0);
  value_t result = value_mod(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 shift ---- */

TEST_F(it_integer_type, i32_shl) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 1);
  value_t b = create_i32_value(vm, 4);
  value_t result = value_shl(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 16);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_shr) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 16);
  value_t b = create_i32_value(vm, 2);
  value_t result = value_shr(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 4);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 unary ---- */

TEST_F(it_integer_type, i32_pos) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, -42);
  value_t result = value_pos(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), -42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_neg) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  value_t result = value_neg(vm, a);

  EXPECT_EQ(*(int32_t *)value_get_data(result), -42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 relational ---- */

TEST_F(it_integer_type, i32_gt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 5);
  value_t result = value_gt(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_lt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 5);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_lt(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_ne) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 20);
  value_t result = value_ne(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_ge) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_ge(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_le) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 5);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_le(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 bitwise ---- */

TEST_F(it_integer_type, i32_band) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0xFF00);
  value_t b = create_i32_value(vm, 0x0FF0);
  value_t result = value_band(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 0x0F00);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_bor) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0xFF00);
  value_t b = create_i32_value(vm, 0x00FF);
  value_t result = value_bor(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), 0xFFFF);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_bxor) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0xFF00);
  value_t b = create_i32_value(vm, 0x0FF0);
  value_t result = value_bxor(vm, a, b);

  EXPECT_EQ(*(int32_t *)value_get_data(result), (int32_t)(0xFF00 ^ 0x0FF0));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_bnot) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0);
  value_t result = value_bnot(vm, a);

  EXPECT_EQ(*(int32_t *)value_get_data(result), ~0);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_lnot) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0);
  value_t result = value_lnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_lnot_nonzero) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  value_t result = value_lnot(vm, a);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 safe_cast ---- */

TEST_F(it_integer_type, i32_safe_cast_to_i32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  type_t i32t = _get_i32_type(vm);
  value_t result = value_safe_cast(vm, a, i32t);

  /* same type → returns self */
  EXPECT_EQ(result, a);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_safe_cast_to_const_i32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  type_t ci32t = _get_const_i32_type(vm);
  value_t result = value_safe_cast(vm, a, ci32t);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_FALSE(type_is_mut(value_get_type(result)));
  EXPECT_EQ(*(int32_t *)value_get_data(result), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_safe_cast_to_void_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  type_t vt = _get_void_type(vm);
  value_t result = value_safe_cast(vm, a, vt);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, const_i32_safe_cast_to_i32_error) {
  vm_t vm = vm_create(allocator);
  type_t ci32t = _get_const_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, ci32t, NULL, true);
  type_t i32t = _get_i32_type(vm);
  value_t result = value_safe_cast(vm, a, i32t);

  /* const → mutable not allowed */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 assignment ---- */

TEST_F(it_integer_type, i32_assign) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0);
  value_t b = create_i32_value(vm, 42);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_EQ(*(int32_t *)value_get_data(a), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, const_i32_assign_error) {
  vm_t vm = vm_create(allocator);
  type_t ci32t = _get_const_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, ci32t, NULL, true);
  value_t b = create_i32_value(vm, 42);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i32 type-level equal/extends ---- */

TEST_F(it_integer_type, i32_type_equal_i32) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_i32_type(vm);
  value_t b = vm_get_i32_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_type_equal_i64_error) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_i32_type(vm);
  value_t b = vm_get_i64_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_i32_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- shadow propagation ---- */

TEST_F(it_integer_type, i32_add_shadow) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, i32t, NULL, true);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_add(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_neg_shadow) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, i32t, NULL, true);
  value_t result = value_neg(vm, a);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- i8 and i16 smoke tests ---- */

TEST_F(it_integer_type, i8_create_and_add) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i8_value(vm, 10);
  value_t b = create_i8_value(vm, 20);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I8);
  EXPECT_EQ(*(int8_t *)value_get_data(result), 30);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i16_create_and_mul) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i16_value(vm, 100);
  value_t b = create_i16_value(vm, 7);
  value_t result = value_mul(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I16);
  EXPECT_EQ(*(int16_t *)value_get_data(result), 700);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-type integer promotion ---- */

TEST_F(it_integer_type, i8_plus_i32_promotes_to_i32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i8_value(vm, 10);
  value_t b = create_i32_value(vm, 100);
  value_t result = value_add(vm, a, b);

  /* i8 + i32 → result is i32 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 110);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_plus_i64_promotes_to_i64) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 100);
  value_t b = create_i64_value(vm, 2000000000LL);
  value_t result = value_add(vm, a, b);

  /* i32 + i64 → result is i64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(result), 2000000100LL);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i8_gt_i32_promotes) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i8_value(vm, 100);
  value_t b = create_i32_value(vm, 50);
  value_t result = value_gt(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_integer_type, i32_band_i64_promotes) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 0xFF00);
  value_t b = create_i64_value(vm, 0x0FF0);
  value_t result = value_band(vm, a, b);

  /* i32 & i64 → result is i64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(result), 0x0F00);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type not supported on void ---- */

TEST_F(it_integer_type, void_add_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t b = create_void_value(vm);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
