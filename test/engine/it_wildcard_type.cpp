#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/str_type.h"
#include "engine/pointer_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/tuple_type.h"
#include "engine/struct_type.h"
#include "engine/nil_type.h"
#include "engine/wildcard_type.h"
#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ---- value_equal with wildcard on the right ---- */

class it_wildcard_type : public CubecTest {
protected:
  type_t _i32(vm_t vm) { return (type_t)value_get_data(vm_get_i32_type(vm)); }
  type_t _f64(vm_t vm) { return (type_t)value_get_data(vm_get_f64_type(vm)); }
  type_t _bool(vm_t vm) { return (type_t)value_get_data(vm_get_bool_type(vm)); }
  type_t _str(vm_t vm) { return (type_t)value_get_data(vm_get_str_type(vm)); }
  type_t _nil(vm_t vm) { return (type_t)value_get_data(vm_get_nil_type(vm)); }
};

TEST_F(it_wildcard_type, i32_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, bool_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, f64_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t a = create_f64_value(vm, 3.14);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, str_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, nil_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t a = create_nil_value(vm);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, pointer_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t ptv = vm_create_pointer_type_value(vm, _i32(vm), true, false);
  value_t a = vm_create_value(vm, (type_t)value_get_data(ptv), NULL, NULL);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, array_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  array_type_t at = (array_type_t)value_get_data(
      vm_create_array_type_value(vm, _i32(vm), create_i32_value(vm, 3), true));
  int32_t v0 = 1, v1 = 2, v2 = 3;
  value_t elems[] = {
    vm_create_value(vm, _i32(vm), &v0, NULL),
    vm_create_value(vm, _i32(vm), &v1, NULL),
    vm_create_value(vm, _i32(vm), &v2, NULL),
  };
  value_t a = create_array_value(vm, at, elems);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, slice_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = (slice_type_t)value_get_data(
      vm_create_slice_type_value(vm, _i32(vm), true));
  value_t a = create_slice_shadow(vm, st, true);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, tuple_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types, _i32(vm));
  vec_push(types, _f64(vm));
  value_t tv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  tuple_type_t tt = (tuple_type_t)value_get_data(tv);

  int32_t a_val = 1;
  double b_val = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _i32(vm), &a_val, NULL),
    vm_create_value(vm, _f64(vm), &b_val, NULL),
  };
  value_t a = create_tuple_value(vm, tt, elems);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

TEST_F(it_wildcard_type, struct_equal_wildcard_right) {
  vm_t vm = vm_create(allocator);
  value_t stv = vm_create_struct_type_value(vm, "Point", true, "<test>");
  ASSERT_FALSE(value_is_abnormal(stv));
  value_t ft_val = create_type_value(vm, _i32(vm), NULL, false);
  vm_struct_add_field(vm, stv, "x", ft_val, true);
  vm_struct_seal(vm, stv);
  struct_type_t st = (struct_type_t)value_get_data(stv);
  int32_t x = 10;
  value_t fields[] = { vm_create_value(vm, _i32(vm), &x, NULL) };
  value_t a = vm_create_struct_value(vm, stv, fields);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

/* ---- wildcard on left does NOT match concrete ---- */

TEST_F(it_wildcard_type, wildcard_equal_i32_left) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_wildcard_value(vm);
  value_t b = create_i32_value(vm, 42);
  value_t eq = value_equal(vm, a, b);
  /* wildcard on left: only equal to another wildcard */
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}

/* ---- wildcard equal wildcard ---- */

TEST_F(it_wildcard_type, wildcard_equal_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_wildcard_value(vm);
  value_t b = vm_get_wildcard_value(vm);
  value_t eq = value_equal(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));
  vm_dispose(vm, allocator);
}
