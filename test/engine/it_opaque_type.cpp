#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/nil_type.h"
#include "engine/opaque_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/pointer_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_opaque_type : public CubecTest {
protected:
  type_t _get_opaque_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_opaque_type(vm));
  }
  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_nil_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_nil_type(vm));
  }
  pointer_type_t _make_i32_ptr(vm_t vm) {
    value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, false);
    return (pointer_type_t)value_get_data(tv);
  }
};

/* ---- Type creation ---- */

TEST_F(it_opaque_type, type_basics) {
  vm_t vm = vm_create(allocator);
  type_t ot = _get_opaque_type(vm);

  EXPECT_EQ(type_get_kind(ot), TYPE_KIND_OPAQUE);
  EXPECT_STREQ(type_get_name(ot), "opaque");
  EXPECT_FALSE(type_is_mut(ot));
  EXPECT_EQ(type_get_size(ot), sizeof(void *));

  vm_dispose(vm, allocator);
}

/* ---- Value creation ---- */

TEST_F(it_opaque_type, create_opaque_value) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t v = create_opaque_value(vm, &x);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_OPAQUE);
  EXPECT_TRUE(value_is_initialized(v));
  void **data = (void **)value_get_data(v);
  EXPECT_EQ(*data, &x);

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, create_opaque_null) {
  vm_t vm = vm_create(allocator);
  value_t v = create_opaque_value(vm, NULL);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_OPAQUE);
  void **data = (void **)value_get_data(v);
  EXPECT_EQ(*data, nullptr);

  vm_dispose(vm, allocator);
}

/* ---- Clone ---- */

TEST_F(it_opaque_type, clone) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t v = create_opaque_value(vm, &x);
  value_t c = value_clone(vm, v);

  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_OPAQUE);
  void **cdata = (void **)value_get_data(c);
  EXPECT_EQ(*cdata, &x);

  vm_dispose(vm, allocator);
}

/* ---- Equal ---- */

TEST_F(it_opaque_type, equal_opaque_opaque) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t a = create_opaque_value(vm, &x);
  value_t b = create_opaque_value(vm, &x);

  value_t eq = value_equal(vm, a, b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, equal_opaque_opaque_different) {
  vm_t vm = vm_create(allocator);
  int x = 42, y = 99;
  value_t a = create_opaque_value(vm, &x);
  value_t b = create_opaque_value(vm, &y);

  value_t eq = value_equal(vm, a, b);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, equal_opaque_nil_null) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  value_t opaque_val = create_opaque_value(vm, NULL);

  /* nil == opaque(NULL) should be true */
  value_t eq = value_equal(vm, nil_val, opaque_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, equal_opaque_int_fails) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t a = create_opaque_value(vm, &x);
  value_t i32_val = create_i32_value(vm, 42);

  value_t eq = value_equal(vm, a, i32_val);
  EXPECT_TRUE(value_is_error(eq));

  vm_dispose(vm, allocator);
}

/* ---- Safe cast ---- */

TEST_F(it_opaque_type, safe_cast_to_opaque) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t v = create_opaque_value(vm, &x);
  type_t ot = _get_opaque_type(vm);

  value_t result = value_safe_cast(vm, v, ot);
  /* opaque -> opaque is identity */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_OPAQUE);

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, safe_cast_to_i32_fails) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t v = create_opaque_value(vm, &x);
  type_t i32_type = _get_i32_type(vm);

  value_t result = value_safe_cast(vm, v, i32_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Pointer -> Opaque safe_cast ---- */

TEST_F(it_opaque_type, pointer_safe_cast_to_opaque) {
  vm_t vm = vm_create(allocator);
  int32_t x = 42;
  pointer_type_t pt = _make_i32_ptr(vm);
  value_t ptr_val = create_pointer_value_from_addr(vm, pt, &x);

  type_t ot = _get_opaque_type(vm);
  value_t result = value_safe_cast(vm, ptr_val, ot);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_OPAQUE);
  void **data = (void **)value_get_data(result);
  EXPECT_EQ(*data, &x);

  vm_dispose(vm, allocator);
}

/* ---- Nil -> Opaque safe_cast ---- */

TEST_F(it_opaque_type, nil_safe_cast_to_opaque) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  type_t ot = _get_opaque_type(vm);

  value_t result = value_safe_cast(vm, nil_val, ot);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_OPAQUE);
  void **data = (void **)value_get_data(result);
  EXPECT_EQ(*data, nullptr);

  vm_dispose(vm, allocator);
}

/* ---- type_equal via value_equal on type values ---- */

TEST_F(it_opaque_type, type_equal_self) {
  vm_t vm = vm_create(allocator);
  value_t ot_val = vm_get_opaque_type(vm);

  value_t eq = value_equal(vm, ot_val, ot_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, type_equal_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t ot_val = vm_get_opaque_type(vm);
  value_t wt_val = vm_get_wildcard_type(vm);

  value_t eq = value_equal(vm, ot_val, wt_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_opaque_type, type_equal_nil_fails) {
  vm_t vm = vm_create(allocator);
  value_t ot_val = vm_get_opaque_type(vm);
  value_t nt_val = vm_get_nil_type(vm);

  /* comparing type values of different kinds returns exception */
  value_t eq = value_equal(vm, ot_val, nt_val);
  EXPECT_TRUE(value_is_error(eq));

  vm_dispose(vm, allocator);
}

/* ---- Pointer == Nil ---- */

TEST_F(it_opaque_type, pointer_equal_nil_null) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);
  value_t ptr_val = create_pointer_value_from_addr(vm, pt, NULL);
  value_t nil_val = create_nil_value(vm);

  /* pointer(NULL) == nil should be true (tested from pointer side) */
  value_t eq = value_equal(vm, ptr_val, nil_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

/* ---- to_string ---- */

TEST_F(it_opaque_type, to_string) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  value_t v = create_opaque_value(vm, &x);

  value_t s = value_to_string(vm, v);
  string_t *sp = (string_t *)value_get_data(s);
  const char *str = string_get(*sp);
  EXPECT_NE(str, nullptr);
  EXPECT_TRUE(strstr(str, "opaque(") != nullptr);

  vm_dispose(vm, allocator);
}
