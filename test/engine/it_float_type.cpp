#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/float_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <cmath>

using ::testing::Test;

class it_float_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_f16_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_f16_type(vm));
  }
  type_t _get_f32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_f32_type(vm));
  }
  type_t _get_f64_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_f64_type(vm));
  }
  type_t _get_const_f32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_const_f32_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
};

/* ---- f32 create ---- */

TEST_F(it_float_type, f32_create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_f32_value(vm, 3.14f);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(v), 3.14f);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_TRUE(value_is_initialized(v));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f64_create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_f64_value(vm, 2.718281828);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 2.718281828);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f16_create_value) {
  vm_t vm = vm_create(allocator);
  /* f16 stores raw bits: 0x3C00 = 1.0 in IEEE 754 half */
  value_t v = create_f16_value(vm, 0x3C00);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F16);
  EXPECT_EQ(*(uint16_t *)value_get_data(v), 0x3C00u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 equal ---- */

TEST_F(it_float_type, f32_equal_same) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_equal_different) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 20.0f);
  value_t result = value_equal(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_equal_f64_same_value) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f64_value(vm, 10.0);
  value_t result = value_equal(vm, a, b);

  /* float promotion: f32 and f64 → compare as f64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_equal_integer_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_i32_value(vm, 10);
  value_t result = value_equal(vm, a, b);

  /* float != integer → error */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_equal_shadow) {
  vm_t vm = vm_create(allocator);
  type_t f32t = _get_f32_type(vm);
  value_t a = vm_create_value_shadow(vm, f32t, NULL, true);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_equal(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 arithmetic ---- */

TEST_F(it_float_type, f32_add) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 20.0f);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 30.0f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_sub) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 50.0f);
  value_t b = create_f32_value(vm, 20.0f);
  value_t result = value_sub(vm, a, b);

  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 30.0f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_mul) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 6.0f);
  value_t b = create_f32_value(vm, 7.0f);
  value_t result = value_mul(vm, a, b);

  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 42.0f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_div) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 4.0f);
  value_t result = value_div(vm, a, b);

  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 2.5f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_div_by_zero_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 0.0f);
  value_t result = value_div(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_mod) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.5f);
  value_t b = create_f32_value(vm, 3.0f);
  value_t result = value_mod(vm, a, b);

  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), fmodf(10.5f, 3.0f));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_mod_by_zero_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 0.0f);
  value_t result = value_mod(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 unary ---- */

TEST_F(it_float_type, f32_pos) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, -42.5f);
  value_t result = value_pos(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), -42.5f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_neg) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.5f);
  value_t result = value_neg(vm, a);

  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), -42.5f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 relational ---- */

TEST_F(it_float_type, f32_gt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 5.0f);
  value_t result = value_gt(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_lt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 5.0f);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_lt(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_ne) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 20.0f);
  value_t result = value_ne(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_ge) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 10.0f);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_ge(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_le) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 5.0f);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_le(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 bitwise/shift unsupported ---- */

TEST_F(it_float_type, f32_band_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t b = create_f32_value(vm, 2.0f);
  value_t result = value_band(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_bor_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t b = create_f32_value(vm, 2.0f);
  value_t result = value_bor(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_bxor_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t b = create_f32_value(vm, 2.0f);
  value_t result = value_bxor(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_bnot_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t result = value_bnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_shl_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t b = create_f32_value(vm, 2.0f);
  value_t result = value_shl(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_shr_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.0f);
  value_t b = create_f32_value(vm, 2.0f);
  value_t result = value_shr(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 lnot ---- */

TEST_F(it_float_type, f32_lnot_zero) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 0.0f);
  value_t result = value_lnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_lnot_nonzero) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.0f);
  value_t result = value_lnot(vm, a);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 safe_cast ---- */

TEST_F(it_float_type, f32_safe_cast_to_f32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.0f);
  type_t f32t = _get_f32_type(vm);
  value_t result = value_safe_cast(vm, a, f32t);

  /* same type → returns self */
  EXPECT_EQ(result, a);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_safe_cast_to_f64) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.5f);
  type_t f64t = _get_f64_type(vm);
  value_t result = value_safe_cast(vm, a, f64t);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(result), 42.5);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_safe_cast_to_const_f32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.0f);
  type_t cf32t = _get_const_f32_type(vm);
  value_t result = value_safe_cast(vm, a, cf32t);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);
  EXPECT_FALSE(type_is_mut(value_get_type(result)));
  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 42.0f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_safe_cast_to_void_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.0f);
  type_t vt = _get_void_type(vm);
  value_t result = value_safe_cast(vm, a, vt);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_safe_cast_to_i32_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 42.0f);
  type_t i32t = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t result = value_safe_cast(vm, a, i32t);

  /* float → integer not allowed */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, const_f32_safe_cast_to_f32_error) {
  vm_t vm = vm_create(allocator);
  type_t cf32t = _get_const_f32_type(vm);
  value_t a = vm_create_value_shadow(vm, cf32t, NULL, true);
  type_t f32t = _get_f32_type(vm);
  value_t result = value_safe_cast(vm, a, f32t);

  /* const → mutable not allowed */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 assignment ---- */

TEST_F(it_float_type, f32_assign) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 0.0f);
  value_t b = create_f32_value(vm, 42.5f);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(a), 42.5f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, const_f32_assign_error) {
  vm_t vm = vm_create(allocator);
  type_t cf32t = _get_const_f32_type(vm);
  value_t a = vm_create_value_shadow(vm, cf32t, NULL, true);
  value_t b = create_f32_value(vm, 42.0f);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_assign_integer_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 0.0f);
  value_t b = create_i32_value(vm, 42);
  value_t result = value_assignment(vm, a, b);

  /* cannot assign integer to float */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f32 type-level equal/extends ---- */

TEST_F(it_float_type, f32_type_equal_f32) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_f32_type(vm);
  value_t b = vm_get_f32_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_type_equal_f64_error) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_f32_type(vm);
  value_t b = vm_get_f64_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_f32_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- shadow propagation ---- */

TEST_F(it_float_type, f32_add_shadow) {
  vm_t vm = vm_create(allocator);
  type_t f32t = _get_f32_type(vm);
  value_t a = vm_create_value_shadow(vm, f32t, NULL, true);
  value_t b = create_f32_value(vm, 10.0f);
  value_t result = value_add(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_neg_shadow) {
  vm_t vm = vm_create(allocator);
  type_t f32t = _get_f32_type(vm);
  value_t a = vm_create_value_shadow(vm, f32t, NULL, true);
  value_t result = value_neg(vm, a);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f64 arithmetic ---- */

TEST_F(it_float_type, f64_add) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f64_value(vm, 1e15);
  value_t b = create_f64_value(vm, 1.0);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(result), 1e15 + 1.0);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f64_mul) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f64_value(vm, 3.0);
  value_t b = create_f64_value(vm, 7.0);
  value_t result = value_mul(vm, a, b);

  EXPECT_DOUBLE_EQ(*(double *)value_get_data(result), 21.0);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-type float promotion ---- */

TEST_F(it_float_type, f32_plus_f64_promotes_to_f64) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 1.5f);
  value_t b = create_f64_value(vm, 2.5);
  value_t result = value_add(vm, a, b);

  /* f32 + f64 → result is f64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(result), 4.0);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f16_plus_f32_promotes_to_f32) {
  vm_t vm = vm_create(allocator);
  /* f16: 0x3C00 = 1.0, 0x4000 = 2.0 */
  value_t a = create_f16_value(vm, 0x3C00);
  value_t b = create_f32_value(vm, 3.0f);
  value_t result = value_add(vm, a, b);

  /* f16 + f32 → result is f32 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 4.0f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f16_plus_f64_promotes_to_f64) {
  vm_t vm = vm_create(allocator);
  /* f16: 0x3C00 = 1.0 */
  value_t a = create_f16_value(vm, 0x3C00);
  value_t b = create_f64_value(vm, 2.5);
  value_t result = value_add(vm, a, b);

  /* f16 + f64 → result is f64 */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(result), 3.5);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f32_gt_f64_promotes) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 100.0f);
  value_t b = create_f64_value(vm, 50.0);
  value_t result = value_gt(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f16 basic operations ---- */

TEST_F(it_float_type, f16_neg) {
  vm_t vm = vm_create(allocator);
  /* 0x3C00 = 1.0 in f16, neg should give -1.0 = 0xBC00 */
  value_t a = create_f16_value(vm, 0x3C00);
  value_t result = value_neg(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F16);
  EXPECT_EQ(*(uint16_t *)value_get_data(result), 0xBC00u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_float_type, f16_add_same) {
  vm_t vm = vm_create(allocator);
  /* 0x4000 = 2.0 in f16 */
  value_t a = create_f16_value(vm, 0x4000);
  value_t b = create_f16_value(vm, 0x4000);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F16);
  /* 2.0 + 2.0 = 4.0 = 0x4400 in f16 */
  EXPECT_EQ(*(uint16_t *)value_get_data(result), 0x4400u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f64 assignment with promotion ---- */

TEST_F(it_float_type, f32_assign_f64_promotes) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f32_value(vm, 0.0f);
  value_t b = create_f64_value(vm, 42.5);
  value_t result = value_assignment(vm, a, b);

  /* assignment coerces rvalue to lvalue type */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(a), 42.5f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- f64 safe_cast to f32 ---- */

TEST_F(it_float_type, f64_safe_cast_to_f32) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f64_value(vm, 3.14);
  type_t f32t = _get_f32_type(vm);
  value_t result = value_safe_cast(vm, a, f32t);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(result), 3.14f);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
