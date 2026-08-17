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

class it_nil_type : public CubecTest {
protected:
  type_t _get_nil_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_nil_type(vm));
  }
  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  pointer_type_t _make_i32_ptr(vm_t vm) {
    value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, false);
    return (pointer_type_t)value_get_data(tv);
  }
};

/* ---- Type creation ---- */

TEST_F(it_nil_type, type_basics) {
  vm_t vm = vm_create(allocator);
  type_t nt = _get_nil_type(vm);

  EXPECT_EQ(type_get_kind(nt), TYPE_KIND_NIL);
  EXPECT_STREQ(type_get_name(nt), "nil");
  EXPECT_FALSE(type_is_mut(nt));
  EXPECT_EQ(type_get_size(nt), sizeof(void *));

  vm_dispose(vm, allocator);
}

/* ---- Value creation ---- */

TEST_F(it_nil_type, create_nil_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_nil_value(vm);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_NIL);
  EXPECT_TRUE(value_is_initialized(v));
  /* nil stores a void* buffer holding NULL */
  void **data = (void **)value_get_data(v);
  EXPECT_NE(data, nullptr);
  EXPECT_EQ(*data, nullptr);

  vm_dispose(vm, allocator);
}

/* ---- Clone ---- */

TEST_F(it_nil_type, clone) {
  vm_t vm = vm_create(allocator);
  value_t v = create_nil_value(vm);
  value_t c = value_clone(vm, v);

  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_NIL);

  vm_dispose(vm, allocator);
}

/* ---- Equal ---- */

TEST_F(it_nil_type, equal_nil_nil) {
  vm_t vm = vm_create(allocator);
  value_t a = create_nil_value(vm);
  value_t b = create_nil_value(vm);

  value_t eq = value_equal(vm, a, b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, equal_nil_pointer_null) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);

  /* create a pointer with NULL address */
  pointer_type_t pt = _make_i32_ptr(vm);
  value_t ptr_val = create_pointer_value_from_addr(vm, pt, NULL);

  value_t eq = value_equal(vm, nil_val, ptr_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, equal_nil_pointer_nonnull) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);

  int32_t x = 42;
  pointer_type_t pt = _make_i32_ptr(vm);
  value_t ptr_val = create_pointer_value_from_addr(vm, pt, &x);

  value_t eq = value_equal(vm, nil_val, ptr_val);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, equal_nil_int) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  value_t i32_val = create_i32_value(vm, 42);

  value_t eq = value_equal(vm, nil_val, i32_val);
  EXPECT_TRUE(value_is_error(eq));

  vm_dispose(vm, allocator);
}

/* ---- Safe cast ---- */

TEST_F(it_nil_type, safe_cast_to_nil) {
  vm_t vm = vm_create(allocator);
  value_t v = create_nil_value(vm);
  type_t nil_type = _get_nil_type(vm);

  value_t result = value_safe_cast(vm, v, nil_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_NIL);

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, safe_cast_to_pointer) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  pointer_type_t pt = _make_i32_ptr(vm);

  value_t result = value_safe_cast(vm, nil_val, (type_t)pt);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_POINTER);
  /* casted pointer should hold NULL */
  void **data = (void **)value_get_data(result);
  EXPECT_EQ(*data, nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, safe_cast_to_opaque) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  type_t opaque_type = (type_t)value_get_data(vm_get_opaque_type(vm));

  value_t result = value_safe_cast(vm, nil_val, opaque_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_OPAQUE);
  void **data = (void **)value_get_data(result);
  EXPECT_EQ(*data, nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, safe_cast_to_i32_fails) {
  vm_t vm = vm_create(allocator);
  value_t nil_val = create_nil_value(vm);
  type_t i32_type = _get_i32_type(vm);

  value_t result = value_safe_cast(vm, nil_val, i32_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- type_equal via value_equal on type values ---- */

TEST_F(it_nil_type, type_equal_self) {
  vm_t vm = vm_create(allocator);
  value_t nt_val = vm_get_nil_type(vm);

  value_t eq = value_equal(vm, nt_val, nt_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, type_equal_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t nt_val = vm_get_nil_type(vm);
  value_t wt_val = vm_get_wildcard_type(vm);

  value_t eq = value_equal(vm, nt_val, wt_val);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_nil_type, type_equal_i32_fails) {
  vm_t vm = vm_create(allocator);
  value_t nt_val = vm_get_nil_type(vm);
  value_t it_val = vm_get_i32_type(vm);

  /* comparing type values of different kinds returns exception */
  value_t eq = value_equal(vm, nt_val, it_val);
  EXPECT_TRUE(value_is_error(eq));

  vm_dispose(vm, allocator);
}

/* ---- to_string ---- */

TEST_F(it_nil_type, to_string) {
  vm_t vm = vm_create(allocator);
  value_t v = create_nil_value(vm);

  value_t s = value_to_string(vm, v);
  string_t *sp = (string_t *)value_get_data(s);
  EXPECT_STREQ(string_get(*sp), "nil");

  vm_dispose(vm, allocator);
}
