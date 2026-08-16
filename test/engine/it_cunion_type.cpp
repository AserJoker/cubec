#include "common/test_common.h"
#include "engine/bool_type.h"
#include "engine/cunion_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/pointer_type.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/wildcard_type.h"
#include "core/string.h"
#include <gtest/gtest.h>
#include <cstring>

using ::testing::Test;

class it_cunion_type : public CubecTest {
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

  /** Create a type value wrapping a primitive type (for vm_cunion_add_field). */
  value_t _make_type_val(vm_t vm, type_t t) {
    type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type_val, t, NULL);
  }

  /** Create an IntOrFloat cunion type value: int_val:i32 | float_val:f64 */
  value_t _make_int_or_float_type(vm_t vm) {
    value_t tv = vm_create_cunion_type_value(vm, "IntOrFloat", true, "<builtin>");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      (void)vm_cunion_add_field(vm, tv, "int_val", ft, true);
    }
    {
      value_t ft = _make_type_val(vm, _get_f64_type(vm));
      (void)vm_cunion_add_field(vm, tv, "float_val", ft, true);
    }
    (void)vm_cunion_seal(vm, tv);
    return tv;
  }

  /** Create an anonymous cunion type value: a:i32 | b:f64 */
  value_t _make_anon_type(vm_t vm) {
    value_t tv = vm_create_cunion_type_value(vm, NULL, true, "<builtin>");
    {
      value_t ft = _make_type_val(vm, _get_i32_type(vm));
      (void)vm_cunion_add_field(vm, tv, "a", ft, true);
    }
    {
      value_t ft = _make_type_val(vm, _get_f64_type(vm));
      (void)vm_cunion_add_field(vm, tv, "b", ft, true);
    }
    (void)vm_cunion_seal(vm, tv);
    return tv;
  }
};

/* ---- Type creation ---- */

TEST_F(it_cunion_type, create_named) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);
  cunion_type_t ct = (cunion_type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind((type_t)ct), TYPE_KIND_CUNION);
  EXPECT_STREQ(type_get_name((type_t)ct), "IntOrFloat");
  EXPECT_TRUE(type_is_mut((type_t)ct));
  EXPECT_TRUE(vm_cunion_is_sealed(vm, tv));
  EXPECT_EQ(vec_get_size(vm_cunion_get_fields(vm, tv)), 2u);

  /* C-compatible: size == max(field sizes) == sizeof(f64) (smaller than union's tag+payload) */
  EXPECT_EQ(type_get_size((type_t)ct), sizeof(double));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, create_anonymous) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_anon_type(vm);
  cunion_type_t ct = (cunion_type_t)value_get_data(tv);

  EXPECT_EQ(type_get_kind((type_t)ct), TYPE_KIND_CUNION);
  EXPECT_STREQ(type_get_name((type_t)ct), nullptr);
  EXPECT_EQ(vec_get_size(vm_cunion_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, seal_prevents_add_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t ft = _make_type_val(vm, _get_i32_type(vm));
  value_t err = vm_cunion_add_field(vm, tv, "z", ft, true);
  EXPECT_EQ(type_get_kind(value_get_type(err)), TYPE_KIND_EXCEPTION);
  EXPECT_EQ(vec_get_size(vm_cunion_get_fields(vm, tv)), 2u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, duplicate_field_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_cunion_type_value(vm, "Dup", true, "<builtin>");
  value_t ft = _make_type_val(vm, _get_i32_type(vm));
  (void)vm_cunion_add_field(vm, tv, "x", ft, true);

  value_t dup = vm_cunion_add_field(vm, tv, "x", ft, true);
  EXPECT_EQ(type_get_kind(value_get_type(dup)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, const_cunion) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_cunion_type_value(vm, "ConstU", false, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    (void)vm_cunion_add_field(vm, tv, "ok", ft, true);
  }
  (void)vm_cunion_seal(vm, tv);
  cunion_type_t ct = (cunion_type_t)value_get_data(tv);

  EXPECT_FALSE(type_is_mut((type_t)ct));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- C-compatible layout: all fields overlap at offset 0, no tag ---- */

TEST_F(it_cunion_type, fields_overlap_offset_zero) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  vec_t fields = vm_cunion_get_fields(vm, tv);
  size_t fc = vec_get_size(fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(fields, i);
    EXPECT_EQ(field_info_get_offset(fi), 0u);
  }

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, no_tag_in_value_layout) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  /* value size must equal the max field size only (no tag field) */
  cunion_type_t ct = (cunion_type_t)value_get_data(tv);
  EXPECT_EQ(ct->base.size, sizeof(double));

  value_t iv = create_i32_value(vm, 42);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);
  EXPECT_EQ(type_get_size(value_get_type(uv)), sizeof(double));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_cunion_type, create_value) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 42);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  EXPECT_FALSE(value_is_shadow(uv));
  EXPECT_TRUE(value_is_initialized(uv));
  EXPECT_EQ(type_get_kind(value_get_type(uv)), TYPE_KIND_CUNION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t uv = vm_create_cunion_shadow(vm, tv, false);
  EXPECT_TRUE(value_is_shadow(uv));
  EXPECT_FALSE(value_is_initialized(uv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, create_value_unknown_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 1);
  value_t uv = vm_create_cunion_value(vm, tv, "nonexistent", iv);
  EXPECT_EQ(type_get_kind(value_get_type(uv)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_field / set_field: raw, no result wrapping, no active-variant tracking ---- */

TEST_F(it_cunion_type, get_field_no_result_wrapping) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 42);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  /* get_field returns the raw field value (no result wrapping, no narrowing) */
  value_t got = value_get_field(vm, uv, "int_val");
  EXPECT_EQ(type_get_kind(value_get_type(got)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, set_field_overwrites_shared_region) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 42);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  /* write a float into the same shared memory — C reinterpret semantics */
  value_t fv = create_f64_value(vm, 3.5);
  value_t result = value_set_field(vm, uv, "float_val", fv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* int_val and float_val share the same bytes; the i32 now holds the low
   * 4 bytes of the f64 bit pattern (no active-variant tracking). */
  double d = 3.5;
  int32_t raw_i32;
  memcpy(&raw_i32, &d, sizeof(int32_t));

  value_t got_int = value_get_field(vm, uv, "int_val");
  EXPECT_EQ(type_get_kind(value_get_type(got_int)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(got_int), raw_i32);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, get_field_not_found) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 1);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t result = value_get_field(vm, uv, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, set_field_const_rejected) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_cunion_type_value(vm, "ConstU2", false, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    (void)vm_cunion_add_field(vm, tv, "x", ft, true);
  }
  (void)vm_cunion_seal(vm, tv);

  value_t iv = create_i32_value(vm, 1);
  value_t uv = vm_create_cunion_value(vm, tv, "x", iv);

  value_t nv = create_i32_value(vm, 2);
  value_t result = value_set_field(vm, uv, "x", nv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- member_addr: C-compatible, offset 0, no active-variant check ---- */

TEST_F(it_cunion_type, member_addr_offset_zero) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t fv = create_f64_value(vm, 3.5);
  value_t uv = vm_create_cunion_value(vm, tv, "float_val", fv);

  value_t ptr = value_member_addr(vm, uv, "float_val");
  EXPECT_EQ(type_get_kind(value_get_type(ptr)), TYPE_KIND_POINTER);

  /* C-compatible: no active-variant check; pointer points at offset 0.
   * Dereferencing must read back the value stored in the shared region. */
  value_t deref = value_deref_get(vm, ptr);
  EXPECT_EQ(type_get_kind(value_get_type(deref)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(deref), 3.5);

  /* writing through the pointer updates the same overlapping bytes */
  value_t new_fv = create_f64_value(vm, 9.25);
  (void)value_deref_set(vm, ptr, new_fv);
  value_t got = value_get_field(vm, uv, "float_val");
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(got), 9.25);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, member_addr_unknown_field) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 1);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t ptr = value_member_addr(vm, uv, "nonexistent");
  EXPECT_EQ(type_get_kind(value_get_type(ptr)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal: byte-for-byte comparison of the whole overlapping region ---- */

TEST_F(it_cunion_type, equal_same) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv1 = create_i32_value(vm, 42);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv1);

  value_t iv2 = create_i32_value(vm, 42);
  value_t u2 = vm_create_cunion_value(vm, tv, "int_val", iv2);

  value_t eq = value_equal(vm, u1, u2);
  EXPECT_EQ(type_get_kind(value_get_type(eq)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, equal_different) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv1 = create_i32_value(vm, 42);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv1);

  value_t iv2 = create_i32_value(vm, 43);
  value_t u2 = vm_create_cunion_value(vm, tv, "int_val", iv2);

  value_t eq = value_equal(vm, u1, u2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, equal_kind_mismatch) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 42);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t u2 = create_bool_value(vm, true);
  value_t eq = value_equal(vm, u1, u2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal / type_extends: structural, cunion-only ---- */

TEST_F(it_cunion_type, type_equal_structural) {
  vm_t vm = vm_create(allocator);
  value_t t1 = _make_int_or_float_type(vm);

  /* structurally identical type built separately */
  value_t t2 = vm_create_cunion_type_value(vm, "Other", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    (void)vm_cunion_add_field(vm, t2, "int_val", ft, true);
  }
  {
    value_t ft = _make_type_val(vm, _get_f64_type(vm));
    (void)vm_cunion_add_field(vm, t2, "float_val", ft, true);
  }
  (void)vm_cunion_seal(vm, t2);

  type_t a = (type_t)value_get_data(t1);
  type_t b = (type_t)value_get_data(t2);
  value_t eq = type_get_vtable(a).type_equal(vm, a, b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, type_equal_field_count_mismatch) {
  vm_t vm = vm_create(allocator);
  value_t t1 = _make_int_or_float_type(vm);

  value_t t2 = vm_create_cunion_type_value(vm, "One", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    (void)vm_cunion_add_field(vm, t2, "only", ft, true);
  }
  (void)vm_cunion_seal(vm, t2);

  type_t a = (type_t)value_get_data(t1);
  type_t b = (type_t)value_get_data(t2);
  value_t eq = type_get_vtable(a).type_equal(vm, a, b);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);
  type_t a = (type_t)value_get_data(tv);

  type_t wildcard = (type_t)value_get_data(vm_get_wildcard_type(vm));
  value_t ext = type_get_vtable(a).type_extends(vm, a, wildcard);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, type_extends_non_cunion_false) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);
  type_t a = (type_t)value_get_data(tv);

  value_t ext = type_get_vtable(a).type_extends(vm, a, (type_t)value_get_data(vm_get_i32_type(vm)));
  EXPECT_FALSE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast: alias of assignment, no tag remap ---- */

TEST_F(it_cunion_type, safe_cast_identical) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 42);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t casted = value_safe_cast(vm, u1, (type_t)value_get_data(tv));
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_CUNION);

  /* bytes preserved */
  value_t got = value_get_field(vm, casted, "int_val");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_cunion_type, safe_cast_incompatible) {
  vm_t vm = vm_create(allocator);
  value_t t1 = _make_int_or_float_type(vm);

  value_t t2 = vm_create_cunion_type_value(vm, "Diff", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_bool_type(vm));
    (void)vm_cunion_add_field(vm, t2, "flag", ft, true);
  }
  (void)vm_cunion_seal(vm, t2);

  value_t iv = create_i32_value(vm, 1);
  value_t u1 = vm_create_cunion_value(vm, t1, "int_val", iv);

  value_t casted = value_safe_cast(vm, u1, (type_t)value_get_data(t2));
  EXPECT_EQ(type_get_kind(value_get_type(casted)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- clone: whole-region memcpy ---- */

TEST_F(it_cunion_type, clone) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 99);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t u2 = value_clone(vm, u1);
  EXPECT_FALSE(value_is_shadow(u2));
  EXPECT_TRUE(value_is_initialized(u2));

  value_t got = value_get_field(vm, u2, "int_val");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  /* independent memory: mutating u1 doesn't affect u2 */
  value_t nv = create_i32_value(vm, 7);
  (void)value_set_field(vm, u1, "int_val", nv);
  value_t got2 = value_get_field(vm, u2, "int_val");
  EXPECT_EQ(*(int32_t *)value_get_data(got2), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment: whole-region memcpy ---- */

TEST_F(it_cunion_type, assignment) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv1 = create_i32_value(vm, 11);
  value_t u1 = vm_create_cunion_value(vm, tv, "int_val", iv1);

  value_t iv2 = create_i32_value(vm, 22);
  value_t u2 = vm_create_cunion_value(vm, tv, "int_val", iv2);

  value_t result = value_assignment(vm, u1, u2);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  value_t got = value_get_field(vm, u1, "int_val");
  EXPECT_EQ(*(int32_t *)value_get_data(got), 22);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_cunion_type, to_string) {
  vm_t vm = vm_create(allocator);
  value_t tv = _make_int_or_float_type(vm);

  value_t iv = create_i32_value(vm, 1);
  value_t uv = vm_create_cunion_value(vm, tv, "int_val", iv);

  value_t s = value_to_string(vm, uv);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);
  /* should mention the type name and both field names */
  const char *cstr = string_get(*(string_t *)value_get_data(s));
  EXPECT_NE(strstr(cstr, "IntOrFloat"), nullptr);
  EXPECT_NE(strstr(cstr, "int_val"), nullptr);
  EXPECT_NE(strstr(cstr, "float_val"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
