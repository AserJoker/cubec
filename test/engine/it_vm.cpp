#include "engine/vm.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_vm : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);
};

TEST_F(it_vm, create_and_dispose) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm, nullptr);
  EXPECT_NE(vm_get_slots(vm), nullptr);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_vm, slot_map_accessible) {
  vm_t vm = vm_create(allocator);
  int x = 42;
  slot_id_t id = slotmap_insert(vm_get_slots(vm), &x);
  void *ptr = slotmap_get(vm_get_slots(vm), id);
  EXPECT_EQ(ptr, &x);
  slotmap_remove(vm_get_slots(vm), id);
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
