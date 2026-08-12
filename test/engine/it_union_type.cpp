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
#include "engine/union_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_union_type : public CubecTest {
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

  /** Create a Result union: ok:i32 | err:str */
  union_type_t _make_result_type(vm_t vm) {
    union_type_t ut = union_type_create(allocator, "Result", true);
    union_type_add_field(allocator, ut, "ok", _get_i32_type(vm));
    union_type_add_field(allocator, ut, "err", _get_str_type(vm));
    union_type_seal(ut);
    vec_push(vm_get_current_scope(vm)->types, ut);
    return ut;
  }

  /** Create an anonymous union: a:i32 | b:f64 */
  union_type_t _make_anon_type(vm_t vm) {
    union_type_t ut = union_type_create(allocator, NULL, true);
    union_type_add_field(allocator, ut, "a", _get_i32_type(vm));
    union_type_add_field(allocator, ut, "b", _get_f64_type(vm));
    union_type_seal(ut);
    vec_push(vm_get_current_scope(vm)->types, ut);
    return ut;
  }

  /** Create an IntOrFloat union: int_val:i32 | float_val:f64 */
  union_type_t _make_int_or_float_type(vm_t vm) {
    union_type_t ut = union_type_create(allocator, "IntOrFloat", true);
    union_type_add_field(allocator, ut, "int_val", _get_i32_type(vm));
    union_type_add_field(allocator, ut, "float_val", _get_f64_type(vm));
    union_type_seal(ut);
    vec_push(vm_get_current_scope(vm)->types, ut);
    return ut;
  }
};

/* ---- Type creation ---- */

TEST_F(it_union_type, create_named) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  EXPECT_EQ(type_get_kind((type_t)ut), TYPE_KIND_UNION);
  EXPECT_STREQ(type_get_name((type_t)ut), "Result");
  EXPECT_TRUE(type_is_mut((type_t)ut));
  EXPECT_TRUE(union_type_is_sealed(ut));
  EXPECT_EQ(vec_get_size(union_type_get_fields(ut)), 2u);

  /* size >= sizeof(uint32_t) + sizeof(int32_t) */
  EXPECT_GT(type_get_size((type_t)ut), sizeof(int32_t));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_anonymous) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_anon_type(vm);

  EXPECT_EQ(type_get_kind((type_t)ut), TYPE_KIND_UNION);
  EXPECT_STREQ(type_get_name((type_t)ut), nullptr);
  EXPECT_EQ(vec_get_size(union_type_get_fields(ut)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, seal_prevents_add_field) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  union_type_add_field(allocator, ut, "z", _get_i32_type(vm));
  EXPECT_EQ(vec_get_size(union_type_get_fields(ut)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, const_union) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = union_type_create(allocator, "ConstResult", false);
  union_type_add_field(allocator, ut, "ok", _get_i32_type(vm));
  union_type_seal(ut);
  vec_push(vm_get_current_scope(vm)->types, ut);

  EXPECT_FALSE(type_is_mut((type_t)ut));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_union_type, create_value_ok) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  EXPECT_FALSE(value_is_shadow(uv));
  EXPECT_TRUE(value_is_initialized(uv));
  EXPECT_EQ(type_get_kind(value_get_type(uv)), TYPE_KIND_UNION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_value_err) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t err_val = create_str_value(vm, "not found");
  value_t uv = create_union_value(vm, ut, 1, err_val);

  EXPECT_TRUE(value_is_initialized(uv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t uv = create_union_shadow(vm, ut, false);
  EXPECT_TRUE(value_is_shadow(uv));
  EXPECT_FALSE(value_is_initialized(uv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_field / set_field ---- */

TEST_F(it_union_type, get_field_active) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t got = value_get_field(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, get_field_inactive_error) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  /* "err" is inactive → error */
  value_t result = value_get_field(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, set_field_switches_tag) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  /* start with ok=42 */
  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  /* switch to err="fail" — this changes tag */
  value_t err_val = create_str_value(vm, "fail");
  value_t result = value_set_field(vm, uv, "err", err_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* now "ok" should be inactive */
  value_t got_ok = value_get_field(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got_ok)), TYPE_KIND_ERROR);

  /* "err" should be active */
  value_t got_err = value_get_field(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(got_err)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, get_field_not_found) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 1);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t result = value_get_field(vm, uv, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, set_field_const_union_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = union_type_create(allocator, "ConstResult", false);
  union_type_add_field(allocator, ut, "ok", _get_i32_type(vm));
  union_type_seal(ut);
  vec_push(vm_get_current_scope(vm)->types, ut);

  value_t ok_val = create_i32_value(vm, 10);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t new_ok = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, uv, "ok", new_ok);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- member_addr ---- */

TEST_F(it_union_type, member_addr_active) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t addr = value_member_addr(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_POINTER);

  value_t derefed = value_deref_get(vm, addr);
  EXPECT_EQ(type_get_kind(value_get_type(derefed)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(derefed), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, member_addr_inactive_error) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t addr = value_member_addr(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer auto-deref ---- */

TEST_F(it_union_type, pointer_get_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t ptr = value_addrof(vm, uv);
  value_t got = value_get_field(vm, ptr, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, pointer_set_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 10);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t ptr = value_addrof(vm, uv);
  value_t new_ok = create_i32_value(vm, 77);
  value_t result = value_set_field(vm, ptr, "ok", new_ok);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  value_t got = value_get_field(vm, uv, "ok");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 77);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal (duck typing) ---- */

TEST_F(it_union_type, type_equal_same_structure) {
  vm_t vm = vm_create(allocator);
  union_type_t ut1 = _make_result_type(vm);
  union_type_t ut2 = _make_result_type(vm);

  vtable_t vt = type_get_vtable((type_t)ut1);
  value_t teq = vt.type_equal(vm, (type_t)ut1, (type_t)ut2);
  EXPECT_TRUE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, type_equal_different_fields) {
  vm_t vm = vm_create(allocator);
  union_type_t ut1 = _make_result_type(vm);
  union_type_t ut2 = _make_anon_type(vm);

  vtable_t vt = type_get_vtable((type_t)ut1);
  value_t teq = vt.type_equal(vm, (type_t)ut1, (type_t)ut2);
  EXPECT_FALSE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_union_type, equal_same_active_field) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok1 = create_i32_value(vm, 42);
  value_t uv1 = create_union_value(vm, ut, 0, ok1);

  value_t ok2 = create_i32_value(vm, 42);
  value_t uv2 = create_union_value(vm, ut, 0, ok2);

  value_t eq = value_equal(vm, uv1, uv2);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, equal_different_active_field) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv1 = create_union_value(vm, ut, 0, ok_val);

  value_t err_val = create_str_value(vm, "fail");
  value_t uv2 = create_union_value(vm, ut, 1, err_val);

  value_t eq = value_equal(vm, uv1, uv2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast (tag remapping) ---- */

TEST_F(it_union_type, safe_cast_same_type) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t casted = value_safe_cast(vm, uv, (type_t)ut);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_UNION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, safe_cast_tag_remapping) {
  vm_t vm = vm_create(allocator);
  union_type_t ut1 = _make_result_type(vm);
  /* same structure, different instance → same field order */
  union_type_t ut2 = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut1, 0, ok_val);

  /* safe_cast to ut2 — tag should map correctly */
  value_t casted = value_safe_cast(vm, uv, (type_t)ut2);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_UNION);

  /* "ok" should still be active */
  value_t got = value_get_field(vm, casted, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, safe_cast_different_type_rejected) {
  vm_t vm = vm_create(allocator);
  union_type_t ut1 = _make_result_type(vm);
  union_type_t ut2 = _make_anon_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut1, 0, ok_val);

  value_t casted = value_safe_cast(vm, uv, (type_t)ut2);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_union_type, assignment) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv1 = create_union_value(vm, ut, 0, ok_val);

  value_t err_val = create_str_value(vm, "x");
  value_t uv2 = create_union_value(vm, ut, 1, err_val);

  value_t result = value_assignment(vm, uv2, uv1);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* uv2 should now have ok=42 active */
  value_t got = value_get_field(vm, uv2, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone ---- */

TEST_F(it_union_type, clone_value) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t cloned = value_clone(vm, uv);
  EXPECT_TRUE(value_is_initialized(cloned));

  value_t got = value_get_field(vm, cloned, "ok");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, clone_shadow) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t uv = create_union_shadow(vm, ut, false);
  value_t cloned = value_clone(vm, uv);
  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_FALSE(value_is_initialized(cloned));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_union_type, to_string) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t s = value_to_string(vm, uv);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- props / methods ---- */

TEST_F(it_union_type, add_prop_and_get) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  union_type_add_prop(vm, ut, "count", prop_val, false);

  value_t found = (value_t)strmap_find(union_type_get_props(ut), "count");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(found), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, methods_registration) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t method_val = create_i32_value(vm, 42);
  union_type_add_prop(vm, ut, "unwrap", method_val, true);

  value_t prop_val = create_i32_value(vm, 7);
  union_type_add_prop(vm, ut, "count", prop_val, false);

  EXPECT_NE(strmap_find(union_type_get_props(ut), "unwrap"), nullptr);
  EXPECT_NE(strmap_find(union_type_get_methods(ut), "unwrap"), nullptr);
  EXPECT_NE(strmap_find(union_type_get_props(ut), "count"), nullptr);
  EXPECT_EQ(strmap_find(union_type_get_methods(ut), "count"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, member_call_no_method_error) {
  vm_t vm = vm_create(allocator);
  union_type_t ut = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = create_union_value(vm, ut, 0, ok_val);

  value_t argv[] = {};
  value_t result = value_member_call(vm, uv, "nonexistent", 0, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
