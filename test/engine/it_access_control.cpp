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

  /** Create a type value wrapping i32 type (temporary, for vm_union_add_field). */
  value_t _make_type_val(vm_t vm, type_t t) {
    type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
    return value_create(vm_get_allocator(vm), type_type_val, t, false);
  }

  /** Create a struct with module_id="/foo", pub x:i32, private y:i32 */
  value_t _make_foo_struct(vm_t vm) {
    value_t tv = vm_create_struct_type_value(vm, "Foo", true, "/foo");
    (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);   /* pub */
    (void)vm_struct_add_field(vm, tv, "y", vm_get_i32_type(vm), false);  /* private */
    (void)vm_struct_seal(vm, tv);
    return tv;
  }

  /** Create a union type value with module_id="/bar", pub Ok:i32, private Err:i32 */
  value_t _make_bar_union(vm_t vm) {
    value_t tv = vm_create_union_type_value(vm, "Bar", true, "/bar");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      (void)vm_union_add_field(vm, tv, "Ok", ft, true);   /* pub */
    }
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      (void)vm_union_add_field(vm, tv, "Err", ft, false); /* private */
    }
    (void)vm_union_seal(vm, tv);
    return tv;
  }
};

/* ---- Struct: same module access ---- */

TEST_F(it_access_control, struct_same_module_get_private_field_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

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
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  vm_set_current_module_id(vm, "/foo");
  value_t new_y = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "y", new_y);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_same_module_member_addr_private_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  vm_set_current_module_id(vm, "/foo");
  value_t addr = value_member_addr(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_POINTER);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Struct: cross-module access ---- */

TEST_F(it_access_control, struct_cross_module_get_pub_field_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

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
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  /* access from different module → private field rejected */
  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  vm_set_current_module_id(vm, "/other");
  value_t new_y = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, sv, "y", new_y);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_member_addr_private_rejected) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  vm_set_current_module_id(vm, "/other");
  value_t addr = value_member_addr(vm, sv, "y");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_pub_field_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

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
  value_t type_val = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, type_val, "secret", prop_val, false, false); /* private prop */

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "secret");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_get_pub_prop_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, type_val, "count", prop_val, false, true); /* pub prop */

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_cross_module_set_private_prop_rejected) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_struct_add_prop(vm, type_val, "secret", prop_val, false, false); /* private */

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
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

  value_t ptr = value_addrof(vm, sv);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, ptr, "y");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_pointer_cross_module_get_pub_ok) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t vx = create_i32_value(vm, 10);
  value_t vy = create_i32_value(vm, 20);
  value_t fields[] = {vx, vy};
  value_t sv = vm_create_struct_value(vm, type_val, fields);

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
  value_t tv = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "Ok", ok_val);

  vm_set_current_module_id(vm, "/bar");
  value_t got = value_get_field(vm, uv, "Err");
  /* tag=0 (Ok) but Err is private — same module: access control passes,
     but tag=0 != Err index → result with error variant (tag=1) */
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_UNION);
  uint32_t tag = *(uint32_t *)value_get_data(got);
  EXPECT_EQ(tag, 1u); /* error variant */

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_same_module_set_private_field_ok) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "Ok", ok_val);

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
  value_t tv = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "Ok", ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, uv, "Ok");
  /* get_field returns result[i32, error], Ok is active → ok variant */
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_UNION);
  uint32_t tag = *(uint32_t *)value_get_data(got);
  EXPECT_EQ(tag, 0u); /* ok variant */

  /* verify inner value via get_field_raw */
  value_t raw = value_get_field_raw(vm, uv, "Ok");
  EXPECT_EQ(type_get_kind(value_get_type(raw)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(raw), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_get_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t err_val = create_i32_value(vm, 1);
  value_t uv = vm_create_union_value(vm, tv, "Err", err_val); /* tag=1 = Err */

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_field(vm, uv, "Err");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_set_private_field_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "Ok", ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t err_val = create_i32_value(vm, 1);
  value_t result = value_set_field(vm, uv, "Err", err_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_member_addr_private_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "Ok", ok_val);

  vm_set_current_module_id(vm, "/other");
  value_t addr = value_member_addr(vm, uv, "Err");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Union: prop access control ---- */

TEST_F(it_access_control, union_cross_module_get_private_prop_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_union_add_prop(vm, tv, "secret", prop_val, false, false); /* private */

  union_type_t ut = (union_type_t)value_get_data(tv);
  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  vm_set_current_module_id(vm, "/other");
  value_t got = value_get_prop(vm, type_val, "secret");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_cross_module_get_pub_prop_ok) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t prop_val = create_i32_value(vm, 42);
  (void)vm_union_add_prop(vm, tv, "count", prop_val, false, true); /* pub */

  union_type_t ut = (union_type_t)value_get_data(tv);
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
  value_t stv = vm_create_struct_type_value(vm, "BuiltinType", true, "<builtin>");
  (void)vm_struct_add_field(vm, stv, "data", vm_get_i32_type(vm), false); /* private */
  (void)vm_struct_seal(vm, stv);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = vm_create_struct_value(vm, stv, fields);

  /* from /user module, builtin's private field should be rejected */
  vm_set_current_module_id(vm, "/user");
  value_t got = value_get_field(vm, sv, "data");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, builtin_private_field_from_builtin_ok) {
  vm_t vm = vm_create(allocator);
  value_t stv2 = vm_create_struct_type_value(vm, "BuiltinType", true, "<builtin>");
  (void)vm_struct_add_field(vm, stv2, "data", vm_get_i32_type(vm), false); /* private */
  (void)vm_struct_seal(vm, stv2);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = vm_create_struct_value(vm, stv2, fields);

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
  value_t stv3 = vm_create_struct_type_value(vm, "BuiltinType", true, "<builtin>");
  (void)vm_struct_add_field(vm, stv3, "data", vm_get_i32_type(vm), true); /* pub */
  (void)vm_struct_seal(vm, stv3);

  value_t dv = create_i32_value(vm, 42);
  value_t fields[] = {dv};
  value_t sv = vm_create_struct_value(vm, stv3, fields);

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
  value_t type_val = _make_foo_struct(vm);

  EXPECT_TRUE(vm_struct_is_field_pub(vm, type_val, "x"));
  EXPECT_FALSE(vm_struct_is_field_pub(vm, type_val, "y"));
  EXPECT_FALSE(vm_struct_is_field_pub(vm, type_val, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, struct_is_prop_pub) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);

  value_t pub_prop = create_i32_value(vm, 1);
  value_t priv_prop = create_i32_value(vm, 2);
  (void)vm_struct_add_prop(vm, type_val, "pub_count", pub_prop, false, true);
  (void)vm_struct_add_prop(vm, type_val, "priv_count", priv_prop, false, false);

  EXPECT_TRUE(vm_struct_is_prop_pub(vm, type_val, "pub_count"));
  EXPECT_FALSE(vm_struct_is_prop_pub(vm, type_val, "priv_count"));
  EXPECT_FALSE(vm_struct_is_prop_pub(vm, type_val, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_is_field_pub) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  EXPECT_TRUE(vm_union_is_field_pub(vm, tv, "Ok"));
  EXPECT_FALSE(vm_union_is_field_pub(vm, tv, "Err"));
  EXPECT_FALSE(vm_union_is_field_pub(vm, tv, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_is_prop_pub) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);

  value_t pub_prop = create_i32_value(vm, 1);
  value_t priv_prop = create_i32_value(vm, 2);
  (void)vm_union_add_prop(vm, tv, "pub_count", pub_prop, false, true);
  (void)vm_union_add_prop(vm, tv, "priv_count", priv_prop, false, false);

  EXPECT_TRUE(vm_union_is_prop_pub(vm, tv, "pub_count"));
  EXPECT_FALSE(vm_union_is_prop_pub(vm, tv, "priv_count"));
  EXPECT_FALSE(vm_union_is_prop_pub(vm, tv, "nonexistent"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- module_id accessor ---- */

TEST_F(it_access_control, struct_get_module_id) {
  vm_t vm = vm_create(allocator);
  value_t type_val = _make_foo_struct(vm);
  EXPECT_STREQ(vm_struct_get_module_id(vm, type_val), "/foo");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_access_control, union_get_module_id) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_bar_union(vm);
  EXPECT_STREQ(vm_union_get_module_id(vm, tv), "/bar");

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
