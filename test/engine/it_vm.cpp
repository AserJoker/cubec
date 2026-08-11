#include "engine/vm.h"
#include "engine/type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_vm : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _make_i32_type() {
    return type_create(allocator, TYPE_KIND_I32, "i32", 4, 4, false,
                       (vtable_t){0});
  }
};

TEST_F(it_vm, create_and_dispose) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm, nullptr);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_vm, global_scope_created) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm_get_global_scope(vm), nullptr);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_vm, modules_empty_initially) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm_get_modules(vm), nullptr);
  EXPECT_EQ(strmap_get_size(vm_get_modules(vm)), 0u);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_vm, create_value_with_data) {
  vm_t vm = vm_create(allocator);
  type_t i32 = _make_i32_type();
  int32_t val = 42;
  value_t v = vm_create_value(vm, i32, &val, NULL);

  EXPECT_NE(v, nullptr);
  EXPECT_EQ(value_get_type(v), i32);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_NE(value_get_data(v), &val);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  /* value is in current_scope->values, disposed by vm_dispose */
  vm_dispose(vm, allocator);
  allocator_free(allocator, &i32);
  delete_allocator(allocator);
}

TEST_F(it_vm, create_value_shadow) {
  vm_t vm = vm_create(allocator);
  type_t i32 = _make_i32_type();
  value_t v = vm_create_value_shadow(vm, i32, NULL, true);

  EXPECT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_FALSE(value_is_own(v));
  EXPECT_EQ(value_get_data(v), nullptr);

  /* value is in current_scope->values, disposed by vm_dispose */
  vm_dispose(vm, allocator);
  allocator_free(allocator, &i32);
  delete_allocator(allocator);
}
