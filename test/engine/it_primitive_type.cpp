#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
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
    return (type_t)value_get_data(vm_get_error_type(vm));
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
  value_t b = vm_create_value_shadow(vm, void_type, NULL);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, bool_extends_not_supported_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = create_bool_value(vm, false);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Void value_equal/value_extends (not supported) ---- */

TEST_F(it_primitive_type, void_equal_not_supported_error) {
  vm_t vm = vm_create(allocator);
  type_t void_type = _get_void_type(vm);
  value_t a = vm_create_value_shadow(vm, void_type, NULL);
  value_t b = vm_create_value_shadow(vm, void_type, NULL);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_extends_not_supported_error) {
  vm_t vm = vm_create(allocator);
  type_t void_type = _get_void_type(vm);
  value_t a = vm_create_value_shadow(vm, void_type, NULL);
  value_t b = vm_create_value_shadow(vm, void_type, NULL);
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

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
  /* kind mismatch: void vs wildcard — dispatcher returns error */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_equal_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_bool_type(vm);
  /* kind mismatch: void vs bool */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

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
  /* kind mismatch: void vs wildcard — dispatcher returns error */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, void_type_extends_bool) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_void_type(vm);
  value_t b = vm_get_bool_type(vm);
  /* kind mismatch */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

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
  /* kind mismatch: bool vs wildcard */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

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
  /* kind mismatch */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, error_type_equal_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_error_type(vm);
  value_t b = vm_get_error_type(vm);
  /* error type_equal is NULL → _type_equal returns error */
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_primitive_type, error_type_extends_not_supported) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_error_type(vm);
  value_t b = vm_get_error_type(vm);
  /* error type_extends is NULL → _type_extends returns error */
  value_t result = value_extends(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
