#include "engine/vm.h"
#include "engine/type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_vm : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  static value_t _dummy_clone(allocator_t alloc, value_t obj) {
    size_t sz = type_get_size(value_get_type(obj));
    void *new_data = allocator_alloc(alloc, sz);
    memcpy(new_data, value_get_data(obj), sz);
    return value_create(alloc, value_get_type(obj), new_data, true);
  }

  static void _dummy_dispose(allocator_t alloc, value_t obj) {
    void *d = value_get_data(obj);
    allocator_free(alloc, &d);
  }

  type_t _make_i32_type() {
    type_t t = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
    t->kind = TYPE_KIND_I32;
    t->name = "i32";
    t->size = 4;
    t->align = 4;
    t->vtable = (vtable_t){.clone = _dummy_clone, .dispose = _dummy_dispose};
    return t;
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
  value_t v = vm_create_value(vm, i32, &val);

  EXPECT_NE(v, nullptr);
  EXPECT_EQ(value_get_type(v), i32);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_NE(value_get_data(v), &val);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  value_dispose(v, allocator);
  allocator_free(allocator, &i32);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_vm, create_value_ref) {
  vm_t vm = vm_create(allocator);
  type_t i32 = _make_i32_type();
  int32_t val = 7;
  value_t owner = vm_create_value(vm, i32, &val);
  value_t ref = vm_create_value_ref(vm, i32, value_get_data(owner));

  EXPECT_NE(ref, nullptr);
  EXPECT_FALSE(value_is_own(ref));
  EXPECT_EQ(value_get_data(ref), value_get_data(owner));

  /* ref must be disposed before owner since it borrows owner's data */
  value_dispose(ref, allocator);
  value_dispose(owner, allocator);
  allocator_free(allocator, &i32);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
