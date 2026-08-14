#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_struct_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  /** Create a Point struct type with fields x:i32, y:i32 */
  value_t _make_point_type(vm_t vm) {
    value_t tv = vm_create_struct_type_value(vm, "Point", true, "<builtin>");
    (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
    (void)vm_struct_add_field(vm, tv, "y", vm_get_i32_type(vm), true);
    (void)vm_struct_seal(vm, tv);
    return tv;
  }

  /** Create an anonymous struct with fields a:i32, b:f64 */
  value_t _make_anon_type(vm_t vm) {
    value_t tv = vm_create_struct_type_value(vm, NULL, true, "<builtin>");
    (void)vm_struct_add_field(vm, tv, "a", vm_get_i32_type(vm), true);
    (void)vm_struct_add_field(vm, tv, "b", vm_get_f64_type(vm), true);
    (void)vm_struct_seal(vm, tv);
    return tv;
  }

  /** Create a Point3D struct type (x:i32, y:i32, z:i32) — extends Point */
  value_t _make_point3d_type(vm_t vm) {
    value_t tv = vm_create_struct_type_value(vm, "Point3D", true, "<builtin>");
    (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
    (void)vm_struct_add_field(vm, tv, "y", vm_get_i32_type(vm), true);
    (void)vm_struct_add_field(vm, tv, "z", vm_get_i32_type(vm), true);
    (void)vm_struct_seal(vm, tv);
    return tv;
  }
};

/* ---- Type creation ---- */

TEST_F(it_struct_type, create_named) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);
  type_t st = (type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind(st), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name(st), "Point");
  EXPECT_TRUE(type_is_mut(st));
  EXPECT_TRUE(vm_struct_is_sealed(vm, tv));
  EXPECT_EQ(vec_get_size(vm_struct_get_fields(vm, tv)), 2u);

  /* field offsets */
  field_info_t fx = vm_struct_find_field(vm, tv, "x");
  field_info_t fy = vm_struct_find_field(vm, tv, "y");
  ASSERT_NE(fx, nullptr);
  ASSERT_NE(fy, nullptr);
  EXPECT_STREQ(field_info_get_name(fx), "x");
  EXPECT_EQ(field_info_get_offset(fx), 0u);
  EXPECT_STREQ(field_info_get_name(fy), "y");
  EXPECT_EQ(field_info_get_offset(fy), 4u);

  /* size = align_up(8, 4) = 8 */
  EXPECT_EQ(type_get_size(st), 8u);
  EXPECT_EQ(type_get_align(st), 4u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, create_anonymous) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_anon_type(vm);
  type_t st = (type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind(st), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name(st), nullptr); /* anonymous */
  EXPECT_EQ(vec_get_size(vm_struct_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, seal_prevents_add_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  /* trying to add field after seal should not crash */
  (void)vm_struct_add_field(vm, tv, "z", vm_get_i32_type(vm), true);
  /* field count should still be 2 */
  EXPECT_EQ(vec_get_size(vm_struct_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, const_struct) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_struct_type_value(vm, "ConstPoint", false, "<builtin>");
  (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, tv);

  EXPECT_FALSE(type_is_mut((type_t)value_get_data(tv)));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_struct_type, create_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  EXPECT_FALSE(value_is_shadow(sv));
  EXPECT_TRUE(value_is_initialized(sv));
  EXPECT_EQ(type_get_kind(value_get_type(sv)), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t sv = vm_create_struct_shadow(vm, tv, false);
  EXPECT_TRUE(value_is_shadow(sv));
  EXPECT_FALSE(value_is_initialized(sv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_field / set_field ---- */

TEST_F(it_struct_type, get_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 42);
  value_t vy = create_i32_value(vm, 99);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t got_x = value_get_field(vm, sv, "x");
  EXPECT_EQ(type_get_kind(value_get_type(got_x)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got_x), 42);

  value_t got_y = value_get_field(vm, sv, "y");
  EXPECT_EQ(*(int32_t *)value_get_data(got_y), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, set_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t new_x = create_i32_value(vm, 77);
  value_t result = value_set_field(vm, sv, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* verify */
  value_t got = value_get_field(vm, sv, "x");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 77);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, get_field_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t result = value_get_field(vm, sv, "z");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, set_field_const_struct_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_struct_type_value(vm, "ConstPoint", false, "<builtin>");
  (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, tv);

  value_t vx = create_i32_value(vm, 10);
  value_t fields[] = {vx};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t new_x = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- member_addr ---- */

TEST_F(it_struct_type, member_addr) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 42);
  value_t vy = create_i32_value(vm, 99);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t addr = value_member_addr(vm, sv, "x");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_POINTER);

  /* deref should give us the field value */
  value_t derefed = value_deref_get(vm, addr);
  EXPECT_EQ(type_get_kind(value_get_type(derefed)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(derefed), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer auto-deref ---- */

TEST_F(it_struct_type, pointer_get_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 42);
  value_t vy = create_i32_value(vm, 99);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  /* take address of struct → *Point */
  value_t ptr = value_addrof(vm, sv);
  EXPECT_EQ(type_get_kind(value_get_type(ptr)), TYPE_KIND_POINTER);

  /* get_field on pointer auto-derefs */
  value_t got_x = value_get_field(vm, ptr, "x");
  EXPECT_EQ(type_get_kind(value_get_type(got_x)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got_x), 42);

  value_t got_y = value_get_field(vm, ptr, "y");
  EXPECT_EQ(*(int32_t *)value_get_data(got_y), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_set_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  /* take address of struct */
  value_t ptr = value_addrof(vm, sv);

  /* set_field on pointer auto-derefs and writes through */
  value_t new_x = create_i32_value(vm, 77);
  value_t result = value_set_field(vm, ptr, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* verify original struct was modified */
  value_t got = value_get_field(vm, sv, "x");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 77);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_set_field_const_struct_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_struct_type_value(vm, "ConstPoint", false, "<builtin>");
  (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, tv);

  value_t vx = create_i32_value(vm, 10);
  value_t fields[] = {vx};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t ptr = value_addrof(vm, sv);

  /* set_field on pointer to const struct → auto-deref gives const Point → rejected */
  value_t new_x = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, ptr, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_get_field_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t ptr = value_addrof(vm, sv);
  value_t result = value_get_field(vm, ptr, "z");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer upcast ---- */

TEST_F(it_struct_type, pointer_safe_cast_upcast) {
  vm_t vm = vm_create(allocator);
  value_t point_tv  = _make_point_type(vm);
  value_t point3d_tv = _make_point3d_type(vm);
  type_t point  = (type_t)value_get_data(point_tv);
  type_t point3d = (type_t)value_get_data(point3d_tv);

  /* Point3D extends Point (subset: x:i32, y:i32) */
  vtable_t vt3d = type_get_vtable(point3d);
  value_t ext = vt3d.type_extends(vm, point3d, point);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  ASSERT_TRUE(*(bool *)value_get_data(ext));

  /* create *Point3D pointer */
  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t vz = create_i32_value(vm, 3);
  value_t fields3d[] = {vx, vy, vz};
  value_t sv3d = vm_create_struct_value(vm, point3d_tv, fields3d);
  value_t ptr3d = value_addrof(vm, sv3d);

  /* create *Point type */
  value_t ptr_tv = vm_create_pointer_type_value(vm, point, true, false);
  pointer_type_t ptr_point_type = (pointer_type_t)value_get_data(ptr_tv);

  /* safe_cast *Point3D → *Point (upcast) */
  value_t ptr_point = value_safe_cast(vm, ptr3d, (type_t)ptr_point_type);
  EXPECT_EQ(type_get_kind(value_get_type(ptr_point)), TYPE_KIND_POINTER);

  /* access base fields through upcasted pointer */
  value_t got_x = value_get_field(vm, ptr_point, "x");
  EXPECT_EQ(type_get_kind(value_get_type(got_x)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got_x), 1);

  value_t got_y = value_get_field(vm, ptr_point, "y");
  EXPECT_EQ(*(int32_t *)value_get_data(got_y), 2);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_safe_cast_downcast_rejected) {
  vm_t vm = vm_create(allocator);
  value_t point_tv   = _make_point_type(vm);
  value_t point3d_tv = _make_point3d_type(vm);
  type_t point   = (type_t)value_get_data(point_tv);
  type_t point3d = (type_t)value_get_data(point3d_tv);

  /* create *Point pointer */
  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, point_tv, fields);
  value_t ptr = value_addrof(vm, sv);

  /* create *Point3D type */
  value_t ptr3d_tv = vm_create_pointer_type_value(vm, point3d, true, false);
  pointer_type_t ptr3d_type = (pointer_type_t)value_get_data(ptr3d_tv);

  /* safe_cast *Point → *Point3D (downcast) should be rejected */
  value_t result = value_safe_cast(vm, ptr, (type_t)ptr3d_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_safe_cast_const_ptr_to_mut_ptr) {
  vm_t vm = vm_create(allocator);
  value_t point_tv = _make_point_type(vm);
  type_t point = (type_t)value_get_data(point_tv);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, point_tv, fields);

  /* create const *Point (pointer itself is const) */
  value_t cptr_tv = vm_create_pointer_type_value(vm, point, false, false);
  pointer_type_t const_ptr_type = (pointer_type_t)value_get_data(cptr_tv);
  value_t const_ptr = create_pointer_value(vm, const_ptr_type, sv);

  /* create *Point type (mutable pointer) */
  value_t mptr_tv = vm_create_pointer_type_value(vm, point, true, false);
  pointer_type_t mut_ptr_type = (pointer_type_t)value_get_data(mptr_tv);

  /* const *Point → *Point: const is on the pointer variable, not pointee.
   * Copying the address is safe — pointer is a trivial u64. */
  value_t result = value_safe_cast(vm, const_ptr, (type_t)mut_ptr_type);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_POINTER);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_assignment_upcast) {
  vm_t vm = vm_create(allocator);
  value_t point_tv   = _make_point_type(vm);
  value_t point3d_tv = _make_point3d_type(vm);

  /* create *Point3D rvalue */
  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t vz = create_i32_value(vm, 30);
  value_t fields3d[] = {vx, vy, vz};
  value_t sv3d = vm_create_struct_value(vm, point3d_tv, fields3d);
  value_t ptr3d = value_addrof(vm, sv3d);

  /* create *Point lvalue with actual data */
  value_t dx = create_i32_value(vm, 0);
  value_t dy = create_i32_value(vm, 0);
  value_t fields_p[] = {dx, dy};
  value_t sv_point = vm_create_struct_value(vm, point_tv, fields_p);
  value_t ptr_point = value_addrof(vm, sv_point);

  /* assign *Point3D → *Point (upcast assignment) */
  value_t result = value_assignment(vm, ptr_point, ptr3d);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* verify through the *Point pointer — now points to Point3D's data */
  value_t got_x = value_get_field(vm, ptr_point, "x");
  EXPECT_EQ(*(int32_t *)value_get_data(got_x), 10);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, pointer_assignment_shadow) {
  vm_t vm = vm_create(allocator);
  value_t point_tv   = _make_point_type(vm);
  value_t point3d_tv = _make_point3d_type(vm);
  type_t point = (type_t)value_get_data(point_tv);

  /* create *Point3D rvalue */
  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t vz = create_i32_value(vm, 30);
  value_t fields3d[] = {vx, vy, vz};
  value_t sv3d = vm_create_struct_value(vm, point3d_tv, fields3d);
  value_t ptr3d = value_addrof(vm, sv3d);

  /* create *Point shadow lvalue */
  value_t sptr_tv = vm_create_pointer_type_value(vm, point, true, false);
  pointer_type_t ptr_point_type = (pointer_type_t)value_get_data(sptr_tv);
  value_t ptr_shadow = create_pointer_shadow(vm, ptr_point_type, false);

  /* shadow assignment: mark initialized, data stays NULL */
  value_t result = value_assignment(vm, ptr_shadow, ptr3d);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(ptr_shadow));
  EXPECT_TRUE(value_is_shadow(ptr_shadow)); /* data still NULL */

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal (duck typing) ---- */

TEST_F(it_struct_type, type_equal_same_structure) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_point_type(vm);
  value_t tv2 = _make_point_type(vm);
  type_t st1 = (type_t)value_get_data(tv1);
  type_t st2 = (type_t)value_get_data(tv2);

  value_t eq = value_equal(vm,
      vm_create_value_shadow(vm, st1, NULL, true),
      vm_create_value_shadow(vm, st2, NULL, true));

  /* duck typing: same fields → equal */
  vtable_t vt = type_get_vtable(st1);
  value_t teq = vt.type_equal(vm, st1, st2);
  EXPECT_EQ(type_get_kind(value_get_type(teq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, type_equal_different_fields) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_point_type(vm);
  value_t tv2 = _make_anon_type(vm);
  type_t st1 = (type_t)value_get_data(tv1);
  type_t st2 = (type_t)value_get_data(tv2);

  vtable_t vt = type_get_vtable(st1);
  value_t teq = vt.type_equal(vm, st1, st2);
  EXPECT_FALSE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_struct_type, assignment) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields1[] = {vx, vy};
  value_t sv1 = vm_create_struct_value(vm, tv, fields1);

  value_t vx2 = create_i32_value(vm, 0);
  value_t vy2 = create_i32_value(vm, 0);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = vm_create_struct_value(vm, tv, fields2);

  value_t result = value_assignment(vm, sv2, sv1);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(sv2));

  /* verify field values */
  value_t got = value_get_field(vm, sv2, "x");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 10);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast ---- */

TEST_F(it_struct_type, safe_cast_same_type) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 5);
  value_t vy = create_i32_value(vm, 6);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t casted = value_safe_cast(vm, sv, (type_t)value_get_data(tv));
  /* same type → returns self */
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, safe_cast_different_type_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_point_type(vm);
  value_t tv2 = _make_anon_type(vm);

  value_t vx = create_i32_value(vm, 5);
  value_t vy = create_i32_value(vm, 6);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv1, fields);

  value_t casted = value_safe_cast(vm, sv, (type_t)value_get_data(tv2));
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone ---- */

TEST_F(it_struct_type, clone_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  /* test type clone first */
  type_t cloned_type = value_type_clone(vm, (type_t)value_get_data(tv));
  ASSERT_NE(cloned_type, nullptr);
  EXPECT_EQ(type_get_kind(cloned_type), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, clone_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t sv = vm_create_struct_shadow(vm, tv, false);
  value_t cloned = value_clone(vm, sv);
  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_FALSE(value_is_initialized(cloned));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_struct_type, equal_same_values) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx1 = create_i32_value(vm, 10);
  value_t vy1 = create_i32_value(vm, 20);
  value_t fields1[] = {vx1, vy1};
  value_t sv1 = vm_create_struct_value(vm, tv, fields1);

  value_t vx2 = create_i32_value(vm, 10);
  value_t vy2 = create_i32_value(vm, 20);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = vm_create_struct_value(vm, tv, fields2);

  value_t eq = value_equal(vm, sv1, sv2);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, equal_different_values) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx1 = create_i32_value(vm, 10);
  value_t vy1 = create_i32_value(vm, 20);
  value_t fields1[] = {vx1, vy1};
  value_t sv1 = vm_create_struct_value(vm, tv, fields1);

  value_t vx2 = create_i32_value(vm, 99);
  value_t vy2 = create_i32_value(vm, 20);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = vm_create_struct_value(vm, tv, fields2);

  value_t eq = value_equal(vm, sv1, sv2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_struct_type, to_string_named) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t s = value_to_string(vm, sv);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- props / methods ---- */

TEST_F(it_struct_type, add_prop_and_get) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  /* add a static property */
  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, tv, "count", prop_val, false, true);

  /* verify prop is in props map */
  value_t found = (value_t)strmap_find(vm_struct_get_props(vm, tv), "count");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(found), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, methods_registration) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  /* add a method (is_method=true) */
  value_t method_val = create_i32_value(vm, 42); /* placeholder callable */
  (void)vm_struct_add_prop(vm, tv, "add", method_val, true, true);

  /* add a regular prop (is_method=false) */
  value_t prop_val = create_i32_value(vm, 7);
  (void)vm_struct_add_prop(vm, tv, "count", prop_val, false, true);

  /* verify: "add" is in both props and methods */
  EXPECT_NE(strmap_find(vm_struct_get_props(vm, tv), "add"), nullptr);
  EXPECT_NE(strmap_find(vm_struct_get_methods(vm, tv), "add"), nullptr);

  /* "count" is only in props, not in methods */
  EXPECT_NE(strmap_find(vm_struct_get_props(vm, tv), "count"), nullptr);
  EXPECT_EQ(strmap_find(vm_struct_get_methods(vm, tv), "count"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, member_call_no_method_error) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  /* calling a non-existent method should return error */
  value_t argv[] = {};
  value_t result = value_member_call(vm, sv, "nonexistent", 0, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer member_call auto-deref ---- */

TEST_F(it_struct_type, pointer_member_call_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  value_t ptr = value_addrof(vm, sv);

  /* calling a non-existent method through pointer should auto-deref and return error */
  value_t argv[] = {};
  value_t result = value_member_call(vm, ptr, "nonexistent", 0, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type-level get_prop / set_prop (via TYPE_KIND_TYPE value) ---- */

TEST_F(it_struct_type, type_get_prop_via_type_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  /* add a static property */
  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, tv, "count", prop_val, false, true);

  /* tv is already a TYPE_KIND_TYPE value wrapping the struct type */
  value_t type_val = tv;

  /* value_get_prop on the type value should delegate to _struct_type_get_prop */
  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, type_set_prop_via_type_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  /* add a mutable static property */
  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, tv, "count", prop_val, false, true);

  value_t type_val = tv;

  /* set_prop via type value */
  value_t new_val = create_i32_value(vm, 99);
  value_t result = value_set_prop(vm, type_val, "count", new_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* verify the prop was updated */
  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, type_get_prop_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t type_val = tv;

  value_t got = value_get_prop(vm, type_val, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, instance_get_prop_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, tv, "count", prop_val, false, true);

  /* create a struct instance value */
  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, tv, fields);

  /* get_prop on instance should return error (only TYPE_KIND_TYPE supports it) */
  value_t got = value_get_prop(vm, sv, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Empty struct has size 1 ---- */

TEST_F(it_struct_type, empty_struct_size_is_1) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_struct_type_value(vm, "Empty", true, "<builtin>");
  (void)vm_struct_seal(vm, tv);
  EXPECT_EQ(type_get_size((type_t)value_get_data(tv)), 1u);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Shadow operations ---- */

TEST_F(it_struct_type, shadow_equal) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);
  value_t a = vm_create_struct_shadow(vm, tv, true);
  value_t b = vm_create_struct_shadow(vm, tv, true);
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, shadow_assignment) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);
  value_t a = vm_create_struct_shadow(vm, tv, false);
  value_t b = vm_create_struct_shadow(vm, tv, true);
  value_t result = value_assignment(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));
  EXPECT_TRUE(value_is_shadow(a));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, shadow_safe_cast) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);
  value_t sv = vm_create_struct_shadow(vm, tv, true);
  value_t result = value_safe_cast(vm, sv, (type_t)value_get_data(tv));
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(value_get_type(result), (type_t)value_get_data(tv));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, shadow_to_string) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_point_type(vm);
  value_t sv = vm_create_struct_shadow(vm, tv, true);
  value_t result = value_to_string(vm, sv);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

