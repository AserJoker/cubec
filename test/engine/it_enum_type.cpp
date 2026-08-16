#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/enum_type.h"
#include "engine/bool_type.h"
#include "engine/exception_type.h"
#include "engine/integer_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_enum_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
};

/* ---- enum creation + item domain access ---- */

TEST_F(it_enum_type, create_and_domain_access) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);

  int32_t red = 1, green = 2;
  enum_type_add_item(vm, et, "Red", &red);
  enum_type_add_item(vm, et, "Green", &green);

  /* domain access Color::Red */
  value_t red_item = enum_type_find_item(et, "Red");
  ASSERT_NE(red_item, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(red_item), 1);

  value_t green_item = enum_type_find_item(et, "Green");
  ASSERT_NE(green_item, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(green_item), 2);

  /* underlying buffer is owned by the item (real memory) */
  EXPECT_TRUE(value_is_own(red_item));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- enum == strict isolation ---- */

TEST_F(it_enum_type, equal_same_item_true) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);

  int32_t red = 1;
  enum_type_add_item(vm, et, "Red", &red);
  value_t red_item = enum_type_find_item(et, "Red");
  value_t red_var = create_enum_value(vm, et, &red);

  value_t result = value_equal(vm, red_item, red_var);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_enum_type, equal_different_item_false) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);

  int32_t red = 1, green = 2;
  enum_type_add_item(vm, et, "Red", &red);
  enum_type_add_item(vm, et, "Green", &green);
  value_t red_item = enum_type_find_item(et, "Red");
  value_t green_item = enum_type_find_item(et, "Green");

  value_t result = value_equal(vm, red_item, green_item);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_enum_type, equal_non_enum_is_exception) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);

  int32_t red = 1;
  enum_type_add_item(vm, et, "Red", &red);
  value_t red_item = enum_type_find_item(et, "Red");
  value_t i32_val = create_i32_value(vm, 1);

  value_t result = value_equal(vm, red_item, i32_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_extends: only self + wildcard ---- */

TEST_F(it_enum_type, type_extends_only_self_and_wildcard) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);
  type_t wt = (type_t)value_get_data(vm_get_wildcard_type(vm));

  value_t self_ext = value_extends(vm, enum_tv, enum_tv);
  EXPECT_TRUE(*(bool *)value_get_data(self_ext));

  value_t wild_ext = value_extends(vm, enum_tv,
      create_type_value(vm, wt, NULL, false));
  EXPECT_TRUE(*(bool *)value_get_data(wild_ext));

  /* enum does not extend its underlying type */
  value_t und_ext = value_extends(vm, enum_tv, i32_tv);
  EXPECT_FALSE(*(bool *)value_get_data(und_ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- shadow value semantics ---- */

TEST_F(it_enum_type, shadow_value_equal_returns_shadow) {
  vm_t vm = vm_create(allocator);

  value_t i32_tv = vm_get_i32_type(vm);
  value_t enum_tv = vm_create_enum_type_value(vm, "Color", i32_tv, false, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);

  int32_t red = 1;
  enum_type_add_item(vm, et, "Red", &red);

  /* two shadow enum values (compile-time only, no real memory) */
  value_t a = create_enum_shadow(vm, et, true);
  value_t b = create_enum_shadow(vm, et, true);

  /* equal of two shadows short-circuits to a shadow value (compile-time only) */
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
