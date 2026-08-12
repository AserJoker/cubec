#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_struct_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_f64_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_f64_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }

  /** Create a Point struct type with fields x:i32, y:i32 */
  struct_type_t _make_point_type(vm_t vm) {
    struct_type_t st = struct_type_create(allocator, "Point", true);
    struct_type_add_field(allocator, st, "x", _get_i32_type(vm));
    struct_type_add_field(allocator, st, "y", _get_i32_type(vm));
    struct_type_seal(st);
    /* register in scope->types for proper lifecycle */
    vec_push(vm_get_current_scope(vm)->types, st);
    return st;
  }

  /** Create an anonymous struct with fields a:i32, b:f64 */
  struct_type_t _make_anon_type(vm_t vm) {
    struct_type_t st = struct_type_create(allocator, NULL, true);
    struct_type_add_field(allocator, st, "a", _get_i32_type(vm));
    struct_type_add_field(allocator, st, "b", _get_f64_type(vm));
    struct_type_seal(st);
    vec_push(vm_get_current_scope(vm)->types, st);
    return st;
  }
};

/* ---- Type creation ---- */

TEST_F(it_struct_type, create_named) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  EXPECT_EQ(type_get_kind((type_t)st), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name((type_t)st), "Point");
  EXPECT_TRUE(type_is_mut((type_t)st));
  EXPECT_TRUE(struct_type_is_sealed(st));
  EXPECT_EQ(vec_get_size(struct_type_get_fields(st)), 2u);

  /* field offsets */
  field_info_t fx = struct_type_find_field(st, "x");
  field_info_t fy = struct_type_find_field(st, "y");
  ASSERT_NE(fx, nullptr);
  ASSERT_NE(fy, nullptr);
  EXPECT_STREQ(field_info_get_name(fx), "x");
  EXPECT_EQ(field_info_get_offset(fx), 0u);
  EXPECT_STREQ(field_info_get_name(fy), "y");
  EXPECT_EQ(field_info_get_offset(fy), 4u);

  /* size = align_up(8, 4) = 8 */
  EXPECT_EQ(type_get_size((type_t)st), 8u);
  EXPECT_EQ(type_get_align((type_t)st), 4u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, create_anonymous) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_anon_type(vm);

  EXPECT_EQ(type_get_kind((type_t)st), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name((type_t)st), nullptr); /* anonymous */
  EXPECT_EQ(vec_get_size(struct_type_get_fields(st)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, seal_prevents_add_field) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  /* trying to add field after seal should not crash */
  struct_type_add_field(allocator, st, "z", _get_i32_type(vm));
  /* field count should still be 2 */
  EXPECT_EQ(vec_get_size(struct_type_get_fields(st)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, const_struct) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = struct_type_create(allocator, "ConstPoint", false);
  struct_type_add_field(allocator, st, "x", _get_i32_type(vm));
  struct_type_seal(st);
  vec_push(vm_get_current_scope(vm)->types, st);

  EXPECT_FALSE(type_is_mut((type_t)st));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_struct_type, create_value) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  EXPECT_FALSE(value_is_shadow(sv));
  EXPECT_TRUE(value_is_initialized(sv));
  EXPECT_EQ(type_get_kind(value_get_type(sv)), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t sv = create_struct_shadow(vm, st, false);
  EXPECT_TRUE(value_is_shadow(sv));
  EXPECT_FALSE(value_is_initialized(sv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_field / set_field ---- */

TEST_F(it_struct_type, get_field) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 42);
  value_t vy = create_i32_value(vm, 99);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

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
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

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
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t result = value_get_field(vm, sv, "z");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, set_field_const_struct_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = struct_type_create(allocator, "ConstPoint", false);
  struct_type_add_field(allocator, st, "x", _get_i32_type(vm));
  struct_type_seal(st);
  vec_push(vm_get_current_scope(vm)->types, st);

  value_t vx = create_i32_value(vm, 10);
  value_t fields[] = {vx};
  value_t sv = create_struct_value(vm, st, fields);

  value_t new_x = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- member_addr ---- */

TEST_F(it_struct_type, member_addr) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 42);
  value_t vy = create_i32_value(vm, 99);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t addr = value_member_addr(vm, sv, "x");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_POINTER);

  /* deref should give us the field value */
  value_t derefed = value_deref_get(vm, addr);
  EXPECT_EQ(type_get_kind(value_get_type(derefed)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(derefed), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal (duck typing) ---- */

TEST_F(it_struct_type, type_equal_same_structure) {
  vm_t vm = vm_create(allocator);
  struct_type_t st1 = _make_point_type(vm);
  struct_type_t st2 = _make_point_type(vm);

  value_t eq = value_equal(vm,
      vm_create_value_shadow(vm, (type_t)st1, NULL, true),
      vm_create_value_shadow(vm, (type_t)st2, NULL, true));

  /* duck typing: same fields → equal */
  vtable_t vt = type_get_vtable((type_t)st1);
  value_t teq = vt.type_equal(vm, (type_t)st1, (type_t)st2);
  EXPECT_EQ(type_get_kind(value_get_type(teq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, type_equal_different_fields) {
  vm_t vm = vm_create(allocator);
  struct_type_t st1 = _make_point_type(vm);
  struct_type_t st2 = _make_anon_type(vm);

  vtable_t vt = type_get_vtable((type_t)st1);
  value_t teq = vt.type_equal(vm, (type_t)st1, (type_t)st2);
  EXPECT_FALSE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_struct_type, assignment) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields1[] = {vx, vy};
  value_t sv1 = create_struct_value(vm, st, fields1);

  value_t vx2 = create_i32_value(vm, 0);
  value_t vy2 = create_i32_value(vm, 0);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = create_struct_value(vm, st, fields2);

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
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 5);
  value_t vy = create_i32_value(vm, 6);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t casted = value_safe_cast(vm, sv, (type_t)st);
  /* same type → returns self */
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, safe_cast_different_type_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st1 = _make_point_type(vm);
  struct_type_t st2 = _make_anon_type(vm);

  value_t vx = create_i32_value(vm, 5);
  value_t vy = create_i32_value(vm, 6);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st1, fields);

  value_t casted = value_safe_cast(vm, sv, (type_t)st2);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone ---- */

TEST_F(it_struct_type, clone_value) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  /* test type clone first */
  type_t cloned_type = value_type_clone(vm, (type_t)st);
  ASSERT_NE(cloned_type, nullptr);
  EXPECT_EQ(type_get_kind(cloned_type), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, clone_shadow) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t sv = create_struct_shadow(vm, st, false);
  value_t cloned = value_clone(vm, sv);
  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_FALSE(value_is_initialized(cloned));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_struct_type, equal_same_values) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx1 = create_i32_value(vm, 10);
  value_t vy1 = create_i32_value(vm, 20);
  value_t fields1[] = {vx1, vy1};
  value_t sv1 = create_struct_value(vm, st, fields1);

  value_t vx2 = create_i32_value(vm, 10);
  value_t vy2 = create_i32_value(vm, 20);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = create_struct_value(vm, st, fields2);

  value_t eq = value_equal(vm, sv1, sv2);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, equal_different_values) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx1 = create_i32_value(vm, 10);
  value_t vy1 = create_i32_value(vm, 20);
  value_t fields1[] = {vx1, vy1};
  value_t sv1 = create_struct_value(vm, st, fields1);

  value_t vx2 = create_i32_value(vm, 99);
  value_t vy2 = create_i32_value(vm, 20);
  value_t fields2[] = {vx2, vy2};
  value_t sv2 = create_struct_value(vm, st, fields2);

  value_t eq = value_equal(vm, sv1, sv2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_struct_type, to_string_named) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 1);
  value_t vy = create_i32_value(vm, 2);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t s = value_to_string(vm, sv);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- props / methods ---- */

TEST_F(it_struct_type, add_prop_and_get) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  /* add a static property */
  value_t prop_val = create_i32_value(vm, 42);
  struct_type_add_prop(vm, st, "count", prop_val, false);

  /* verify prop is in props map */
  value_t found = (value_t)strmap_find(struct_type_get_props(st), "count");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(found), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, methods_registration) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  /* add a method (is_method=true) */
  value_t method_val = create_i32_value(vm, 42); /* placeholder callable */
  struct_type_add_prop(vm, st, "add", method_val, true);

  /* add a regular prop (is_method=false) */
  value_t prop_val = create_i32_value(vm, 7);
  struct_type_add_prop(vm, st, "count", prop_val, false);

  /* verify: "add" is in both props and methods */
  EXPECT_NE(strmap_find(struct_type_get_props(st), "add"), nullptr);
  EXPECT_NE(strmap_find(struct_type_get_methods(st), "add"), nullptr);

  /* "count" is only in props, not in methods */
  EXPECT_NE(strmap_find(struct_type_get_props(st), "count"), nullptr);
  EXPECT_EQ(strmap_find(struct_type_get_methods(st), "count"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_struct_type, member_call_no_method_error) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_point_type(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  /* calling a non-existent method should return error */
  value_t argv[] = {};
  value_t result = value_member_call(vm, sv, "nonexistent", 0, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
