#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/error.h"
#include "engine/error_code.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/pointer_type.h"
#include "engine/wildcard_type.h"
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

  /** Create a type value wrapping i32 type (temporary, for vm_union_add_field). */
  value_t _make_type_val(vm_t vm, type_t t) {
    type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
    return value_create(vm_get_allocator(vm), type_type_val, t, false);
  }

  /** Create a Result union type value: ok:i32 | err:str */
  value_t _make_result_type(vm_t vm) {
    value_t tv = vm_create_union_type_value(vm, "Result", true, "<builtin>");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      vm_union_add_field(vm, tv, "ok", ft, true);
      vm_get_allocator(vm); /* no free needed — _ut_add_field clones internally */
    }
    {
      value_t ft = _make_type_val(vm, _get_str_type(vm));
      vm_union_add_field(vm, tv, "err", ft, true);
    }
    vm_union_seal(vm, tv);
    return tv;
  }

  /** Read error_code from an error struct value. */
  uint64_t _get_error_code(vm_t vm, value_t err) {
    value_t error_tv = vm_get_error_type(vm);
    field_info_t fi = (field_info_t)vec_get(vm_struct_get_fields(vm, error_tv), 1);
    uint64_t code;
    memcpy(&code, (uint8_t *)value_get_data(err) + field_info_get_offset(fi),
           sizeof(uint64_t));
    return code;
  }

  /** Read tag from a union value (0 = first field active, 1 = second, etc). */
  uint32_t _read_tag(value_t uv) {
    return *(uint32_t *)value_get_data(uv);
  }

  /** Create an anonymous union type value: a:i32 | b:f64 */
  value_t _make_anon_type(vm_t vm) {
    value_t tv = vm_create_union_type_value(vm, NULL, true, "<builtin>");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      vm_union_add_field(vm, tv, "a", ft, true);
    }
    {
      value_t ft = _make_type_val(vm, _get_f64_type(vm));
      vm_union_add_field(vm, tv, "b", ft, true);
    }
    vm_union_seal(vm, tv);
    return tv;
  }

  /** Create an IntOrFloat union type value: int_val:i32 | float_val:f64 */
  value_t _make_int_or_float_type(vm_t vm) {
    value_t tv = vm_create_union_type_value(vm, "IntOrFloat", true, "<builtin>");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      vm_union_add_field(vm, tv, "int_val", ft, true);
    }
    {
      value_t ft = _make_type_val(vm, _get_f64_type(vm));
      vm_union_add_field(vm, tv, "float_val", ft, true);
    }
    vm_union_seal(vm, tv);
    return tv;
  }
};

/* ---- Type creation ---- */

TEST_F(it_union_type, create_named) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  union_type_t ut = (union_type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind((type_t)ut), TYPE_KIND_UNION);
  EXPECT_STREQ(type_get_name((type_t)ut), "Result");
  EXPECT_TRUE(type_is_mut((type_t)ut));
  EXPECT_TRUE(vm_union_is_sealed(vm, tv));
  EXPECT_EQ(vec_get_size(vm_union_get_fields(vm, tv)), 2u);

  /* size >= sizeof(uint32_t) + sizeof(int32_t) */
  EXPECT_GT(type_get_size((type_t)ut), sizeof(int32_t));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_anonymous) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_anon_type(vm);
  union_type_t ut = (union_type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind((type_t)ut), TYPE_KIND_UNION);
  EXPECT_STREQ(type_get_name((type_t)ut), nullptr);
  EXPECT_EQ(vec_get_size(vm_union_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, seal_prevents_add_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  /* try to add field after seal — should return exception */
  value_t ft = _make_type_val(vm, _get_i32_type(vm));
  value_t err = vm_union_add_field(vm, tv, "z", ft, true);
  EXPECT_NE(err, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(err)), TYPE_KIND_EXCEPTION);
  EXPECT_EQ(vec_get_size(vm_union_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, const_union) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_union_type_value(vm, "ConstResult", false, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, tv, "ok", ft, true);
  }
  vm_union_seal(vm, tv);
  union_type_t ut = (union_type_t)value_get_data(tv);

  EXPECT_FALSE(type_is_mut((type_t)ut));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_union_type, create_value_ok) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  EXPECT_FALSE(value_is_shadow(uv));
  EXPECT_TRUE(value_is_initialized(uv));
  EXPECT_EQ(type_get_kind(value_get_type(uv)), TYPE_KIND_UNION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_value_err) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t err_val = create_str_value(vm, "not found");
  value_t uv = vm_create_union_value(vm, tv, "err", err_val);

  EXPECT_TRUE(value_is_initialized(uv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t uv = vm_create_union_shadow(vm, tv, false);
  EXPECT_TRUE(value_is_shadow(uv));
  EXPECT_FALSE(value_is_initialized(uv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_field / set_field ---- */

TEST_F(it_union_type, get_field_active) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  /* get_field now returns result[i32, error] */
  value_t got = value_get_field(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_UNION);
  EXPECT_EQ(_read_tag(got), 0u); /* ok variant */

  /* get_field_raw bypasses result wrapping */
  value_t raw = value_get_field_raw(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(raw)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(raw), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, get_field_inactive_error) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  /* "err" is inactive → result with error variant (tag=1) */
  value_t result = value_get_field(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_UNION);
  EXPECT_EQ(_read_tag(result), 1u); /* error variant */

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, set_field_switches_tag) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  /* start with ok=42 */
  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  /* switch to err="fail" — this changes tag */
  value_t err_val = create_str_value(vm, "fail");
  value_t result = value_set_field(vm, uv, "err", err_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* now "ok" should be inactive → result with error variant */
  value_t got_ok = value_get_field(vm, uv, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got_ok)), TYPE_KIND_UNION);
  EXPECT_EQ(_read_tag(got_ok), 1u); /* error variant */

  /* "err" should be active → result with ok variant */
  value_t got_err = value_get_field(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(got_err)), TYPE_KIND_UNION);
  EXPECT_EQ(_read_tag(got_err), 0u); /* ok variant */

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, get_field_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 1);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t result = value_get_field(vm, uv, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, set_field_const_union_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_union_type_value(vm, "ConstResult", false, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, tv, "ok", ft, true);
  }
  vm_union_seal(vm, tv);

  value_t ok_val = create_i32_value(vm, 10);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t new_ok = create_i32_value(vm, 99);
  value_t result = value_set_field(vm, uv, "ok", new_ok);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- member_addr ---- */

TEST_F(it_union_type, member_addr_active) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

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
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t addr = value_member_addr(vm, uv, "err");
  EXPECT_EQ(type_get_kind(value_get_type(addr)), TYPE_KIND_STRUCT);
  EXPECT_EQ(_get_error_code(vm, addr), ERROR_CODE_UNION_ADDR_INACTIVE);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer auto-deref ---- */

TEST_F(it_union_type, pointer_get_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t ptr = value_addrof(vm, uv);
  value_t got = value_get_field(vm, ptr, "ok");
  /* pointer auto-derefs, then union get_field returns result */
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_UNION);
  EXPECT_EQ(_read_tag(got), 0u); /* ok variant */

  /* raw access through pointer also works */
  value_t raw = value_get_field_raw(vm, ptr, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(raw)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(raw), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, pointer_set_field_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 10);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t ptr = value_addrof(vm, uv);
  value_t new_ok = create_i32_value(vm, 77);
  value_t result = value_set_field(vm, ptr, "ok", new_ok);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* verify via get_field_raw (bypasses result wrapping) */
  value_t got = value_get_field_raw(vm, uv, "ok");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 77);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal (duck typing) ---- */

TEST_F(it_union_type, type_equal_same_structure) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_result_type(vm);
  value_t tv2 = _make_result_type(vm);
  union_type_t ut1 = (union_type_t)value_get_data(tv1);
  union_type_t ut2 = (union_type_t)value_get_data(tv2);

  vtable_t vt = type_get_vtable((type_t)ut1);
  value_t teq = vt.type_equal(vm, (type_t)ut1, (type_t)ut2);
  EXPECT_TRUE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, type_equal_different_fields) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_result_type(vm);
  value_t tv2 = _make_anon_type(vm);
  union_type_t ut1 = (union_type_t)value_get_data(tv1);
  union_type_t ut2 = (union_type_t)value_get_data(tv2);

  vtable_t vt = type_get_vtable((type_t)ut1);
  value_t teq = vt.type_equal(vm, (type_t)ut1, (type_t)ut2);
  EXPECT_FALSE(*(bool *)value_get_data(teq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_union_type, equal_same_active_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok1 = create_i32_value(vm, 42);
  value_t uv1 = vm_create_union_value(vm, tv, "ok", ok1);

  value_t ok2 = create_i32_value(vm, 42);
  value_t uv2 = vm_create_union_value(vm, tv, "ok", ok2);

  value_t eq = value_equal(vm, uv1, uv2);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, equal_different_active_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv1 = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t err_val = create_str_value(vm, "fail");
  value_t uv2 = vm_create_union_value(vm, tv, "err", err_val);

  value_t eq = value_equal(vm, uv1, uv2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast (tag remapping) ---- */

TEST_F(it_union_type, safe_cast_same_type) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  union_type_t ut = (union_type_t)value_get_data(tv);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t casted = value_safe_cast(vm, uv, (type_t)ut);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_UNION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, safe_cast_tag_remapping) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_result_type(vm);
  value_t tv2 = _make_result_type(vm);
  union_type_t ut1 = (union_type_t)value_get_data(tv1);
  union_type_t ut2 = (union_type_t)value_get_data(tv2);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv1, "ok", ok_val);

  /* safe_cast to ut2 — tag should map correctly */
  value_t casted = value_safe_cast(vm, uv, (type_t)ut2);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_UNION);

  /* "ok" should still be active — verify via get_field_raw */
  value_t got = value_get_field_raw(vm, casted, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, safe_cast_different_type_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv1 = _make_result_type(vm);
  value_t tv2 = _make_anon_type(vm);
  union_type_t ut1 = (union_type_t)value_get_data(tv1);
  union_type_t ut2 = (union_type_t)value_get_data(tv2);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv1, "ok", ok_val);

  value_t casted = value_safe_cast(vm, uv, (type_t)ut2);
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_union_type, assignment) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv1 = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t err_val = create_str_value(vm, "x");
  value_t uv2 = vm_create_union_value(vm, tv, "err", err_val);

  value_t result = value_assignment(vm, uv2, uv1);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* uv2 should now have ok=42 active — verify via get_field_raw */
  value_t got = value_get_field_raw(vm, uv2, "ok");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone ---- */

TEST_F(it_union_type, clone_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t cloned = value_clone(vm, uv);
  EXPECT_TRUE(value_is_initialized(cloned));

  value_t got = value_get_field_raw(vm, cloned, "ok");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, clone_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t uv = vm_create_union_shadow(vm, tv, false);
  value_t cloned = value_clone(vm, uv);
  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_FALSE(value_is_initialized(cloned));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_union_type, to_string) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t s = value_to_string(vm, uv);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- props / methods ---- */

TEST_F(it_union_type, add_prop_and_get) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  vm_union_add_prop(vm, tv, "count", prop_val, false, true);

  value_t found = (value_t)strmap_find(vm_union_get_props(vm, tv), "count");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(found), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, methods_registration) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t method_val = create_i32_value(vm, 42);
  vm_union_add_prop(vm, tv, "unwrap", method_val, true, true);

  value_t prop_val = create_i32_value(vm, 7);
  vm_union_add_prop(vm, tv, "count", prop_val, false, true);

  EXPECT_NE(strmap_find(vm_union_get_props(vm, tv), "unwrap"), nullptr);
  EXPECT_NE(strmap_find(vm_union_get_methods(vm, tv), "unwrap"), nullptr);
  EXPECT_NE(strmap_find(vm_union_get_props(vm, tv), "count"), nullptr);
  EXPECT_EQ(strmap_find(vm_union_get_methods(vm, tv), "count"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, member_call_no_method_error) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t argv[] = {};
  value_t result = value_member_call(vm, uv, "nonexistent", 0, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type-level get_prop / set_prop (via TYPE_KIND_TYPE value) ---- */

TEST_F(it_union_type, type_get_prop_via_type_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  vm_union_add_prop(vm, tv, "count", prop_val, false, true);

  union_type_t ut = (union_type_t)value_get_data(tv);
  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, type_set_prop_via_type_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  vm_union_add_prop(vm, tv, "count", prop_val, false, true);

  union_type_t ut = (union_type_t)value_get_data(tv);
  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  value_t new_val = create_i32_value(vm, 99);
  value_t result = value_set_prop(vm, type_val, "count", new_val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  value_t got = value_get_prop(vm, type_val, "count");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, type_get_prop_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  union_type_t ut = (union_type_t)value_get_data(tv);

  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);

  value_t got = value_get_prop(vm, type_val, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, instance_get_prop_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t prop_val = create_i32_value(vm, 42);
  vm_union_add_prop(vm, tv, "count", prop_val, false, true);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  /* get_prop on instance should return error (only TYPE_KIND_TYPE supports it) */
  value_t got = value_get_prop(vm, uv, "count");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- is_instance ---- */

TEST_F(it_union_type, is_instance_active_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  /* ok=42 → active tag = 0 → i32 */
  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t result = value_is(vm, uv, _get_i32_type(vm));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, is_instance_inactive_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  /* ok=42 → active tag = 0 → i32, NOT str */
  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t result = value_is(vm, uv, _get_str_type(vm));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, is_instance_switched_tag) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  /* err="fail" → active tag = 1 → str */
  value_t err_val = create_str_value(vm, "fail");
  value_t uv = vm_create_union_value(vm, tv, "err", err_val);

  value_t result_i32 = value_is(vm, uv, _get_i32_type(vm));
  EXPECT_FALSE(*(bool *)value_get_data(result_i32));

  value_t result_str = value_is(vm, uv, _get_str_type(vm));
  EXPECT_TRUE(*(bool *)value_get_data(result_str));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, is_instance_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t uv = vm_create_union_shadow(vm, tv, false);
  value_t result = value_is(vm, uv, _get_i32_type(vm));
  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- pointer is_instance auto-deref ---- */

TEST_F(it_union_type, pointer_is_instance_auto_deref) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);

  value_t ok_val = create_i32_value(vm, 42);
  value_t uv = vm_create_union_value(vm, tv, "ok", ok_val);

  value_t ptr = value_addrof(vm, uv);
  value_t result = value_is(vm, ptr, _get_i32_type(vm));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Empty union rejected ---- */

TEST_F(it_union_type, seal_empty_union_returns_false) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_union_type_value(vm, "Empty", true, "<builtin>");
  value_t err = vm_union_seal(vm, tv);
  EXPECT_NE(err, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(err)), TYPE_KIND_EXCEPTION);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Shadow operations ---- */

TEST_F(it_union_type, shadow_equal) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  value_t a = vm_create_union_shadow(vm, tv, true);
  value_t b = vm_create_union_shadow(vm, tv, true);
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, shadow_assignment) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  value_t a = vm_create_union_shadow(vm, tv, false);
  value_t b = vm_create_union_shadow(vm, tv, true);
  value_t result = value_assignment(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));
  EXPECT_TRUE(value_is_shadow(a));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, shadow_safe_cast) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  union_type_t ut = (union_type_t)value_get_data(tv);
  value_t uv = vm_create_union_shadow(vm, tv, true);
  value_t result = value_safe_cast(vm, uv, (type_t)ut);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(value_get_type(result), (type_t)ut);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_union_type, shadow_to_string) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_result_type(vm);
  value_t uv = vm_create_union_shadow(vm, tv, true);
  value_t result = value_to_string(vm, uv);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
