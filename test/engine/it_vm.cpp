#include "engine/vm.h"
#include "engine/type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_vm : public CubecTest {
protected:

  type_t _make_i32_type() {
    return type_create(allocator, TYPE_KIND_I32, "i32", 4, 4, false,
                       (vtable_t){0});
  }
};

TEST_F(it_vm, create_and_dispose) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm, nullptr);
  vm_dispose(vm, allocator);
}

TEST_F(it_vm, global_scope_created) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm_get_global_scope(vm), nullptr);
  vm_dispose(vm, allocator);
}

TEST_F(it_vm, modules_empty_initially) {
  vm_t vm = vm_create(allocator);
  EXPECT_NE(vm_get_modules(vm), nullptr);
  EXPECT_EQ(strmap_get_size(vm_get_modules(vm)), 0u);
  vm_dispose(vm, allocator);
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
}

TEST_F(it_vm, wildcard_value_is_global_unique) {
  vm_t vm = vm_create(allocator);
  value_t wv1 = vm_get_wildcard_value(vm);
  value_t wv2 = vm_get_wildcard_value(vm);

  EXPECT_NE(wv1, nullptr);
  EXPECT_EQ(wv1, wv2); /* same pointer 鈥?global unique */
  EXPECT_EQ(type_get_kind(value_get_type(wv1)), TYPE_KIND_WILDCARD);
  EXPECT_TRUE(value_is_shadow(wv1)); /* no data */

  vm_dispose(vm, allocator);
}

/* ---- Call stack ---- */

TEST_F(it_vm, call_stack_push_pop) {
  vm_t vm = vm_create(allocator);

  vm_push_frame(vm, "main", "entry");
  vm_push_frame(vm, "foo", "calling bar");

  vec_t cs = vm_get_call_stack(vm);
  EXPECT_EQ(vec_get_size(cs), 2u);

  call_frame_t f0 = (call_frame_t)vec_get(cs, 0);
  EXPECT_STREQ(f0->name, "main");
  EXPECT_STREQ(f0->message, "entry");

  call_frame_t f1 = (call_frame_t)vec_get(cs, 1);
  EXPECT_STREQ(f1->name, "foo");
  EXPECT_STREQ(f1->message, "calling bar");

  vm_pop_frame(vm);
  EXPECT_EQ(vec_get_size(cs), 1u);

  vm_pop_frame(vm);
  EXPECT_EQ(vec_get_size(cs), 0u);

  vm_dispose(vm, allocator);
}

TEST_F(it_vm, call_stack_empty_initially) {
  vm_t vm = vm_create(allocator);
  vec_t cs = vm_get_call_stack(vm);
  EXPECT_EQ(cs, nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_vm, call_stack_pop_empty_noop) {
  vm_t vm = vm_create(allocator);
  vm_pop_frame(vm); /* should not crash */

  vm_dispose(vm, allocator);
}

TEST_F(it_vm, call_stack_null_message) {
  vm_t vm = vm_create(allocator);

  vm_push_frame(vm, "init", NULL);
  vec_t cs = vm_get_call_stack(vm);
  call_frame_t f = (call_frame_t)vec_get(cs, 0);
  EXPECT_STREQ(f->name, "init");
  EXPECT_EQ(f->message, nullptr);

  vm_dispose(vm, allocator);
}
