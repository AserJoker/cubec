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
#include "engine/union_type.h"
#include "engine/callable_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_access_control : public CubecTest {
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

  /** Create a struct with module_id="/foo", pub x:i32, private y:i32 */
  struct_type_t _make_foo_struct(vm_t vm) {
    struct_type_t st = struct_type_create(allocator, "Foo", true, "/foo");
    struct_type_add_field(allocator, st, "x", _get_i32_type(vm), true);   /* pub */
    struct_type_add_field(allocator, st, "y", _get_i32_type(vm), false);  /* private */
    struct_type_seal(st);
    vec_push(vm_get_current_scope(vm)->types, st);
    return st;
  }

  /** Create a union with module_id="/bar", pub Ok:i32, private Err:i32 */
  union_type_t _make_bar_union(vm_t vm) {
    union_type_t ut = union_type_create(allocator, "Bar", true, "/bar");
    union_type_add_field(allocator, ut, "Ok", _get_i32_type(vm), true);   /* pub */
    union_type_add_field(allocator, ut, "Err", _get_i32_type(vm), false); /* private */
    union_type_seal(ut);
    vec_push(vm_get_current_scope(vm)->types, ut);
    return ut;
  }
};

/* ---- Struct: same module access ---- */

TEST_F(it_access_control, struct_same_module_get_private_field_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  /* access from same module → private field accessible */
  vm_set_current_module_id(vm, "/foo");
  value_t got = value_get_field(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 20);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_same_module_set_private_field_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  vm_set_current_module_id(vm, "/foo");
  value_t new_y = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "y", new_y);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_same_module_member_addr_private_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  vm_set_current_module_id(vm, "/foo");
  value_t addr = value_member_addr(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_POINTER);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Struct: cross-module access ---- */

TEST_F(it_access_control, struct_cross_module_get_pub_field_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  /* access from different module → pub field accessible */
  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, sv, "x");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 10);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_get_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  /* access from different module → private field rejected */
  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  vm_set_current_module_id(vm, "/other");
  value_t new_y = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "y", new_y);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_member_addr_private_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  vm_set_current_module_id(vm, "/other");
  value_t addr = value_member_addr(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_pub_field_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  vm_set_current_module_id(vm, "/other");
  value_t new_x = create_i32_value(vm, 77);
  value_t result = value_set_field(vm, sv, "x", new_x);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Struct: prop access control ---- */

TEST_F(it_access_control, struct_cross_module_get_private_prop_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  struct_type_add_prop(vm, st, "secret", prop_val, false, false); /* private prop */

  value_t type_val = create_type_value(vm, (type_t)st, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "secret");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_get_pub_prop_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  struct_type_add_prop(vm, st, "count", prop_val, false, true); /* pub prop */

  value_t type_val = create_type_value(vm, (type_t)st, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_private_prop_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  struct_type_add_prop(vm, st, "secret", prop_val, false, false); /* private */

  value_t type_val = create_type_value(vm, (type_t)st, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t new_val = create_i32_value(vm, 99);
  value_t result = value_set_prop(vm, type_val, "secret", new_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Struct: pointer auto-deref access control ---- */

TEST_F(it_access_control, struct_pointer_cross_module_get_private_rejected) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t ptr = value_addrof(vm, sv);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, ptr, "y");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_pointer_cross_module_get_pub_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = create_struct_value(vm, st, fields);

  value_t ptr = value_addrof(vm, sv);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, ptr, "x");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 10);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Union: same module access ---- */

TEST_F(it_access_control, union_same_module_get_private_field_ok) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  vm_set_current_module_id(vm, "/bar");
  value_t got = value_get_field(vm, uv, "Err");
  /* tag=0 (Ok) but Err is private — same module: access control passes,
     but tag=0 != Err index → inactive field error (error struct, TYPE_KIND_STRUCT) */
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_STRUCT);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_same_module_set_private_field_ok) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  vm_set_current_module_id(vm, "/bar");
  /* set private field from same module */
  value_t err_val = create_i32_value(vm, 1);
  value_t result = value_set_field(vm, uv, "Err", err_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Union: cross-module access ---- */

TEST_F(it_access_control, union_cross_module_get_pub_field_ok) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, uv, "Ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_get_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t err_val = create_i32_value(vm, 1);
  value_t uv = create_union_value(vm, ut, 1, err_val); /* tag=1 = Err */

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, uv, "Err");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_set_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t err_val = create_i32_value(vm, 1);
  value_t result = value_set_field(vm, uv, "Err", err_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_member_addr_private_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t addr = value_member_addr(vm, uv, "Err");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Union: prop access control ---- */

TEST_F(it_access_control, union_cross_module_get_private_prop_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t prop_val = create_i32_value(vm, 42);
  union_type_add_prop(vm, ut, "secret", prop_val, false, false); /* private */

  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "secret");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_get_pub_prop_ok) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t prop_val = create_i32_value(vm, 42);
  union_type_add_prop(vm, ut, "count", prop_val, false, true); /* pub */

  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Builtin module: same rules as any module ---- */

TEST_F(it_access_control, builtin_private_field_from_other_module_rejected) {
  vm_t vm = vm_create(allocator);
  /* struct with module_id="<builtin>", private field */
  struct_type_t st = struct_type_create(allocator, "BuiltinType", true, "<builtin>");
  struct_type_add_field(allocator, st, "data", _get_i32_type(vm), false); /* private */
  struct_type_seal(st);
  vec_push(vm_get_current_scope(vm)->types, st);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = create_struct_value(vm, st, fields);

  /* from /user module, builtin's private field should be rejected */
  vm_set_current_module_id(vm, "/user");
  value_t got = value_get_field(vm, sv, "data");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, builtin_private_field_from_builtin_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = struct_type_create(allocator, "BuiltinType", true, "<builtin>");
  struct_type_add_field(allocator, st, "data", _get_i32_type(vm), false); /* private */
  struct_type_seal(st);
  vec_push(vm_get_current_scope(vm)->types, st);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = create_struct_value(vm, st, fields);

  /* from <builtin> module, builtin's private field is accessible */
  vm_set_current_module_id(vm, "<builtin>");
  value_t got = value_get_field(vm, sv, "data");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, builtin_pub_field_from_other_module_ok) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = struct_type_create(allocator, "BuiltinType", true, "<builtin>");
  struct_type_add_field(allocator, st, "data", _get_i32_type(vm), true); /* pub */
  struct_type_seal(st);
  vec_push(vm_get_current_scope(vm)->types, st);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = create_struct_value(vm, st, fields);

  /* pub field from builtin is accessible from any module */
  vm_set_current_module_id(vm, "/user");
  value_t got = value_get_field(vm, sv, "data");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Accessor: is_field_pub / is_prop_pub ---- */

TEST_F(it_access_control, struct_is_field_pub) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  EXPECT_TRUE(struct_type_is_field_pub(st, "x"));
  EXPECT_FALSE(struct_type_is_field_pub(st, "y"));
  EXPECT_FALSE(struct_type_is_field_pub(st, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_is_prop_pub) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);

  value_t pub_prop = create_i32_value(vm, 1);
  value_t priv_prop = create_i32_value(vm, 2);
  struct_type_add_prop(vm, st, "pub_count", pub_prop, false, true);
  struct_type_add_prop(vm, st, "priv_count", priv_prop, false, false);

  EXPECT_TRUE(struct_type_is_prop_pub(st, "pub_count"));
  EXPECT_FALSE(struct_type_is_prop_pub(st, "priv_count"));
  EXPECT_FALSE(struct_type_is_prop_pub(st, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_is_field_pub) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  EXPECT_TRUE(union_type_is_field_pub(ut, "Ok"));
  EXPECT_FALSE(union_type_is_field_pub(ut, "Err"));
  EXPECT_FALSE(union_type_is_field_pub(ut, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_is_prop_pub) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);

  value_t pub_prop = create_i32_value(vm, 1);
  value_t priv_prop = create_i32_value(vm, 2);
  union_type_add_prop(vm, ut, "pub_count", pub_prop, false, true);
  union_type_add_prop(vm, ut, "priv_count", priv_prop, false, false);

  EXPECT_TRUE(union_type_is_prop_pub(ut, "pub_count"));
  EXPECT_FALSE(union_type_is_prop_pub(ut, "priv_count"));
  EXPECT_FALSE(union_type_is_prop_pub(ut, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- module_id accessor ---- */

TEST_F(it_access_control, struct_get_module_id) {
  vm_t vm = vm_create(allocator);
  struct_type_t st = _make_foo_struct(vm);
  EXPECT_STREQ(struct_type_get_module_id(st), "/foo");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_get_module_id) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_bar_union(vm);
  EXPECT_STREQ(union_type_get_module_id(ut), "/bar");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- vm current_module_id ---- */

TEST_F(it_access_control, vm_default_module_id_is_builtin) {
  vm_t vm = vm_create(allocator);
  EXPECT_STREQ(vm_get_current_module_id(vm), "<builtin>");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, vm_set_and_get_current_module_id) {
  vm_t vm = vm_create(allocator);
  vm_set_current_module_id(vm, "/my/module");
  EXPECT_STREQ(vm_get_current_module_id(vm), "/my/module");

  vm_set_current_module_id(vm, "<builtin>");
  EXPECT_STREQ(vm_get_current_module_id(vm), "<builtin>");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
