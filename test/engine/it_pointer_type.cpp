#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/pointer_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_pointer_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_const_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_const_i32_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }

  pointer_type_t _make_i32_ptr(vm_t vm) {
    value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, false);
    return (pointer_type_t)value_get_data(tv);
  }

  pointer_type_t _make_const_i32_ptr(vm_t vm) {
    value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), false, false);
    return (pointer_type_t)value_get_data(tv);
  }

  pointer_type_t _make_volatile_i32_ptr(vm_t vm) {
    value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, true);
    return (pointer_type_t)value_get_data(tv);
  }
};

/* ---- Type creation ---- */

TEST_F(it_pointer_type, create_basic) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  EXPECT_EQ(type_get_kind((type_t)pt), TYPE_KIND_POINTER);
  EXPECT_STREQ(type_get_name((type_t)pt), "* i32");
  EXPECT_TRUE(type_is_mut((type_t)pt));
  EXPECT_EQ(type_get_kind(pointer_type_get_pointee_type(pt)), TYPE_KIND_I32);
  EXPECT_FALSE(pointer_type_is_volatile(pt));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_const_ptr) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_const_i32_ptr(vm);

  EXPECT_STREQ(type_get_name((type_t)pt), "const * i32");
  EXPECT_FALSE(type_is_mut((type_t)pt));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_volatile_ptr) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_volatile_i32_ptr(vm);

  EXPECT_STREQ(type_get_name((type_t)pt), "* volatile i32");
  EXPECT_TRUE(pointer_type_is_volatile(pt));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_const_volatile_ptr) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), false, true);
  pointer_type_t pt = (pointer_type_t)value_get_data(tv);

  EXPECT_STREQ(type_get_name((type_t)pt), "const * volatile i32");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_ptr_to_const) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_pointer_type_value(vm, _get_const_i32_type(vm), true, false);
  pointer_type_t pt = (pointer_type_t)value_get_data(tv);

  /* * const i32 — mutable pointer to const i32 */
  EXPECT_STREQ(type_get_name((type_t)pt), "* const i32");
  EXPECT_TRUE(type_is_mut((type_t)pt));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_pointer_type, create_pointer_value) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  EXPECT_NE(pv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(pv)), TYPE_KIND_POINTER);
  EXPECT_NE(value_get_data(pv), nullptr);

  /* pointer stores the address of target's data */
  void **ptr = (void **)value_get_data(pv);
  EXPECT_EQ(*ptr, value_get_data(target));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_pointer_from_addr) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int x = 99;
  value_t pv = create_pointer_value_from_addr(vm, pt, &x);
  void **ptr = (void **)value_get_data(pv);
  EXPECT_EQ(*ptr, &x);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, create_pointer_shadow) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);
  value_t pv = create_pointer_shadow(vm, pt, false);

  EXPECT_TRUE(value_is_shadow(pv));
  EXPECT_FALSE(value_is_initialized(pv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- value_addrof ---- */

TEST_F(it_pointer_type, addrof_basic) {
  vm_t vm = vm_create(allocator);
  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);

  value_t pv = value_addrof(vm, target);
  EXPECT_EQ(type_get_kind(value_get_type(pv)), TYPE_KIND_POINTER);

  pointer_type_t pt = (pointer_type_t)value_get_type(pv);
  EXPECT_EQ(type_get_kind(pointer_type_get_pointee_type(pt)), TYPE_KIND_I32);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, addrof_void_error) {
  vm_t vm = vm_create(allocator);
  value_t v = create_void_value(vm);
  value_t result = value_addrof(vm, v);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, addrof_type_error) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_get_i32_type(vm);
  value_t result = value_addrof(vm, tv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, addrof_error_error) {
  vm_t vm = vm_create(allocator);
  value_t ev = create_exception_value(vm, "test");
  value_t result = value_addrof(vm, ev);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- deref_get ---- */

TEST_F(it_pointer_type, deref_get_basic) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  value_t derefed = value_deref_get(vm, pv);
  EXPECT_EQ(type_get_kind(value_get_type(derefed)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(derefed), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- deref_set ---- */

TEST_F(it_pointer_type, deref_set_basic) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  int32_t new_val = 100;
  value_t new_v = vm_create_value(vm, _get_i32_type(vm), &new_val, NULL);
  value_t result = value_deref_set(vm, pv, new_v);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* original target's data should be modified */
  EXPECT_EQ(*(int32_t *)value_get_data(target), 100);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, deref_set_const_ptr_error) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_const_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  int32_t new_val = 100;
  value_t new_v = vm_create_value(vm, _get_i32_type(vm), &new_val, NULL);
  value_t result = value_deref_set(vm, pv, new_v);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal ---- */

TEST_F(it_pointer_type, type_equal_same) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt1 = _make_i32_ptr(vm);
  pointer_type_t pt2 = _make_i32_ptr(vm);

  vtable_t vt = type_get_vtable((type_t)pt1);
  value_t eq = vt.type_equal(vm, (type_t)pt1, (type_t)pt2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, type_equal_const_vs_mutable) {
  vm_t vm = vm_create(allocator);
  pointer_type_t mut_pt = _make_i32_ptr(vm);
  pointer_type_t const_pt = _make_const_i32_ptr(vm);

  vtable_t vt = type_get_vtable((type_t)mut_pt);
  value_t eq = vt.type_equal(vm, (type_t)mut_pt, (type_t)const_pt);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, type_equal_volatile_ignored) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);
  pointer_type_t vpt = _make_volatile_i32_ptr(vm);

  /* volatile is silently ignored in equals */
  vtable_t vt = type_get_vtable((type_t)pt);
  value_t eq = vt.type_equal(vm, (type_t)pt, (type_t)vpt);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, type_equal_different_pointee) {
  vm_t vm = vm_create(allocator);
  pointer_type_t i32_pt = _make_i32_ptr(vm);
  value_t tv = vm_create_pointer_type_value(vm, _get_bool_type(vm), true, false);
  pointer_type_t bool_pt = (pointer_type_t)value_get_data(tv);

  vtable_t vt = type_get_vtable((type_t)i32_pt);
  value_t eq = vt.type_equal(vm, (type_t)i32_pt, (type_t)bool_pt);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_extends ---- */

TEST_F(it_pointer_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);
  type_t wc = (type_t)value_get_data(vm_get_wildcard_type(vm));

  vtable_t vt = type_get_vtable((type_t)pt);
  value_t ext = vt.type_extends(vm, (type_t)pt, wc);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_pointer_type, equal_same_addr) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t p1 = create_pointer_value(vm, pt, target);
  value_t p2 = create_pointer_value(vm, pt, target);

  value_t eq = value_equal(vm, p1, p2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, equal_different_addr) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t v1 = 42, v2 = 42;
  value_t t1 = vm_create_value(vm, _get_i32_type(vm), &v1, NULL);
  value_t t2 = vm_create_value(vm, _get_i32_type(vm), &v2, NULL);
  value_t p1 = create_pointer_value(vm, pt, t1);
  value_t p2 = create_pointer_value(vm, pt, t2);

  value_t eq = value_equal(vm, p1, p2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_pointer_type, assignment) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t v1 = 42, v2 = 99;
  value_t t1 = vm_create_value(vm, _get_i32_type(vm), &v1, NULL);
  value_t t2 = vm_create_value(vm, _get_i32_type(vm), &v2, NULL);
  value_t p1 = create_pointer_value(vm, pt, t1);
  value_t p2 = create_pointer_value(vm, pt, t2);

  value_t result = value_assignment(vm, p1, p2);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* p1 now points to t2's data */
  void **ptr1 = (void **)value_get_data(p1);
  EXPECT_EQ(*ptr1, value_get_data(t2));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_pointer_type, assignment_const_ptr_error) {
  vm_t vm = vm_create(allocator);
  pointer_type_t const_pt = _make_const_i32_ptr(vm);
  pointer_type_t mut_pt = _make_i32_ptr(vm);

  int32_t v1 = 42, v2 = 99;
  value_t t1 = vm_create_value(vm, _get_i32_type(vm), &v1, NULL);
  value_t t2 = vm_create_value(vm, _get_i32_type(vm), &v2, NULL);
  value_t cp = create_pointer_value(vm, const_pt, t1);
  value_t mp = create_pointer_value(vm, mut_pt, t2);

  value_t result = value_assignment(vm, cp, mp);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone ---- */

TEST_F(it_pointer_type, clone) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  value_t cloned = value_clone(vm, pv);
  EXPECT_NE(cloned, pv);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_POINTER);

  /* cloned points to same address */
  void **orig_ptr = (void **)value_get_data(pv);
  void **clone_ptr = (void **)value_get_data(cloned);
  EXPECT_EQ(*orig_ptr, *clone_ptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_pointer_type, to_string) {
  vm_t vm = vm_create(allocator);
  pointer_type_t pt = _make_i32_ptr(vm);

  int32_t val = 42;
  value_t target = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t pv = create_pointer_value(vm, pt, target);

  value_t str = value_to_string(vm, pv);
  EXPECT_EQ(type_get_kind(value_get_type(str)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
