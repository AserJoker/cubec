#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_primitive_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  /* helpers to extract inner type_t from vm bootstrap value */
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
  type_t _get_error_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_exception_type(vm));
  }
  type_t _get_wildcard_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_wildcard_type(vm));
  }
};

/* ---- Bool value_equal ---- */

TEST_F(it_primitive_type, bool_equal_same_true) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_equal(vm, a, b);

  type_t rt = value_get_type(result);
  EXPECT_EQ(type_get_kind(rt), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_equal_same_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t b = create_bool_value(vm, false);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_equal_different) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_equal_different_kind_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  /* void value can't be created (no create_void_value), use shadow */
  type_t void_type = _get_void_type(vm);
  value_t b = vm_create_value_shadow(vm, void_type, NULL, true);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_extends_not_supported_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Void value_equal/value_extends (not supported) ---- */

TEST_F(it_primitive_type, void_equal_not_supported_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t b = create_void_value(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_extends_not_supported_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t b = create_void_value(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Type-level equal/extends via type values ---- */

TEST_F(it_primitive_type, void_type_equal_void) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_void_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_equal_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  /* wildcard short-circuit: any type equal to wildcard → true */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_equal_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_bool_type(vm);
  /* kind mismatch: void vs bool */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_extends_void) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_void_type(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  /* wildcard short-circuit: any type extends wildcard → true */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_extends_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_bool_type(vm);
  /* type value's extends delegates to data's type_extends; void does not
   * extend bool → false (not an exception, since the comparison is well-defined) */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_type_equal_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_bool_type(vm);
  value_t b = vm_get_bool_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_type_equal_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_bool_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  /* wildcard short-circuit: any type equal to wildcard → true */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_type_extends_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_bool_type(vm);
  value_t b = vm_get_bool_type(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_bool_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  /* wildcard short-circuit: any type extends wildcard → true */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, error_type_equal_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_exception_type(vm);
  value_t b = vm_get_exception_type(vm);
  /* error type_equal is NULL → _type_equal returns error */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, error_type_extends_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_exception_type(vm);
  value_t b = vm_get_exception_type(vm);
  /* error type_extends is NULL → _type_extends returns error */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool shadow handling ---- */

TEST_F(it_primitive_type, bool_equal_shadow_left) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_equal(vm, a, b);

  /* shadow operand → result is shadow bool */
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_equal_shadow_right) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = create_bool_value(vm, true);
  value_t b = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_equal(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_equal_shadow_both) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t b = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_equal(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool binary operators ---- */

TEST_F(it_primitive_type, bool_band_true_true) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_band(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_band_true_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_band(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_band_false_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t b = create_bool_value(vm, false);
  value_t result = value_band(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bor_true_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_bor(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bor_false_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t b = create_bool_value(vm, false);
  value_t result = value_bor(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bxor_true_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_bxor(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bxor_true_true) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_bxor(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool unary operators ---- */

TEST_F(it_primitive_type, bool_bnot_true) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t result = value_bnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bnot_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t result = value_bnot(vm, a);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_lnot_true) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t result = value_lnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_lnot_false) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t result = value_lnot(vm, a);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool operators with shadow ---- */

TEST_F(it_primitive_type, bool_band_shadow) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_band(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bor_shadow) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = create_bool_value(vm, false);
  value_t b = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_bor(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bxor_shadow) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t b = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_bxor(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_bnot_shadow) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_bnot(vm, a);

  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_lnot_shadow) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_lnot(vm, a);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool operators not supported on void/error ---- */

TEST_F(it_primitive_type, void_band_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t b = create_void_value(vm);
  value_t result = value_band(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, error_lnot_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_exception_type(vm);
  value_t result = value_lnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Bool assignment ---- */

TEST_F(it_primitive_type, bool_assign_value) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, false);
  value_t b = create_bool_value(vm, true);
  value_t result = value_assignment(vm, a, b);

  /* assignment returns void */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(*(bool *)value_get_data(a));
  EXPECT_TRUE(value_is_initialized(a));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_assign_shadow_lvalue) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  /* TDZ shadow: initialized=false */
  value_t a = vm_create_value_shadow(vm, bt, NULL, false);
  value_t b = create_bool_value(vm, true);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_assign_shadow_rvalue) {
  vm_t vm = vm_create(allocator);
  type_t bt = _get_bool_type(vm);
  value_t a = create_bool_value(vm, false);
  value_t b = vm_create_value_shadow(vm, bt, NULL, true);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_assign_kind_mismatch_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  type_t vt = _get_void_type(vm);
  value_t b = vm_create_value_shadow(vm, vt, NULL, true);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, const_bool_assign_error) {
  vm_t vm = vm_create(allocator);
  /* const bool value: initialized=true, mut=false → cannot assign */
  type_t cbt = (type_t)value_get_data(vm_get_const_bool_type(vm));
  value_t a = vm_create_value_shadow(vm, cbt, NULL, true);
  value_t b = create_bool_value(vm, true);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, const_bool_assign_tdz_allowed) {
  vm_t vm = vm_create(allocator);
  /* const bool value: initialized=false (TDZ), mut=false → can assign once */
  type_t cbt = (type_t)value_get_data(vm_get_const_bool_type(vm));
  value_t a = vm_create_value_shadow(vm, cbt, NULL, false);
  value_t b = create_bool_value(vm, true);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_assign_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t b = create_void_value(vm);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Void create/clone/dispose ---- */

TEST_F(it_primitive_type, void_create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_void_value(vm);

  EXPECT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(v));
  EXPECT_EQ(value_get_data(v), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_move) {
  vm_t vm = vm_create(allocator);
  value_t v = create_void_value(vm);
  allocator_t alloc = vm_get_allocator(vm);
  value_t moved = (value_t)alloc_move(alloc, v);

  EXPECT_NE(moved, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(moved)), TYPE_KIND_VOID);
  EXPECT_EQ(value_get_data(moved), nullptr);
  /* source is cleared */
  EXPECT_EQ(value_get_data(v), nullptr);
  EXPECT_FALSE(value_is_own(v));

  allocator_free(alloc, &moved);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
