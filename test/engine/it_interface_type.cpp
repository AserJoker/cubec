/**
 * @file it_interface_type.cpp
 * @brief Integration tests for interface_type_t — compile-time constraint type.
 */
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/callable_type.h"
#include "engine/interface_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_interface_type : public CubecTest {
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

  /** Create a type value wrapping a type_t (temporary, for vm_union_add_field). */
  value_t _make_type_val(vm_t vm, type_t t) {
    type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
    return value_create(vm_get_allocator(vm), type_type_val, t, false);
  }

  /** Create a TYPE_KIND_TYPE value wrapping a callable_type_t (for vm_interface_add_method). */
  value_t _make_callable_type_val(vm_t vm, callable_type_t ct) {
    type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
    return value_create(vm_get_allocator(vm), type_type_val, ct, false);
  }
};

/* ---- Type creation ---- */

TEST_F(it_interface_type, create_basic) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "Drawable", true, "<builtin>");
  ASSERT_NE(tv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(tv)), TYPE_KIND_TYPE);

  interface_type_t it = (interface_type_t)value_get_data(tv);
  EXPECT_EQ(type_get_kind((type_t)it), TYPE_KIND_INTERFACE);
  EXPECT_STREQ(type_get_name((type_t)it), "Drawable");
  EXPECT_TRUE(type_is_mut((type_t)it));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, create_const) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "ConstIface", false, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(tv);
  EXPECT_FALSE(type_is_mut((type_t)it));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Add method + seal ---- */

TEST_F(it_interface_type, add_method_and_seal) {
  vm_t vm = vm_create(allocator);

  /* create interface */
  value_t tv = vm_create_interface_type_value(vm, "Printable", true, "<builtin>");

  /* create a callable_type: () -> void */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  callable_type_t ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                            "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);

  {
    value_t ctv = _make_callable_type_val(vm, ct);
    vm_interface_add_method(vm, tv, "print", ctv);
  }

  /* find method */
  callable_type_t found = vm_interface_find_method(vm, tv, "print");
  ASSERT_NE(found, nullptr);

  /* not found */
  EXPECT_EQ(vm_interface_find_method(vm, tv, "missing"), nullptr);

  /* seal */
  EXPECT_TRUE(vm_interface_is_sealed(vm, tv) == false);
  value_t seal_err = vm_interface_seal(vm, tv);
  EXPECT_EQ(seal_err, nullptr);
  EXPECT_TRUE(vm_interface_is_sealed(vm, tv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, seal_empty_interface_fails) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "Empty", true, "<builtin>");

  /* empty interface cannot be sealed */
  value_t seal_err = vm_interface_seal(vm, tv);
  EXPECT_NE(seal_err, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(seal_err)), TYPE_KIND_EXCEPTION);
  EXPECT_FALSE(vm_interface_is_sealed(vm, tv));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, add_method_after_seal_emits_error) {
  vm_t vm = vm_create(allocator);

  value_t tv = vm_create_interface_type_value(vm, "Sealed", true, "<builtin>");

  /* add one method so seal succeeds */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  callable_type_t ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                            "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);
  {
    value_t ctv = _make_callable_type_val(vm, ct);
    vm_interface_add_method(vm, tv, "do_it", ctv);
  }
  vm_interface_seal(vm, tv);

  /* adding after seal should return exception */
  {
    value_t ctv = _make_callable_type_val(vm, ct);
    value_t err = vm_interface_add_method(vm, tv, "late_method", ctv);
    EXPECT_NE(err, nullptr);
    EXPECT_EQ(type_get_kind(value_get_type(err)), TYPE_KIND_EXCEPTION);
  }

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_methods ---- */

TEST_F(it_interface_type, get_methods_returns_strmap) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "IFoo", true, "<builtin>");

  strmap_t methods = vm_interface_get_methods(vm, tv);
  ASSERT_NE(methods, nullptr);
  EXPECT_EQ(strmap_get_size(methods), 0u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_module_id ---- */

TEST_F(it_interface_type, get_module_id) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "Bar", true, "my/module");
  EXPECT_STREQ(vm_interface_get_module_id(vm, tv), "my/module");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- vm_interface_check_extends — struct implements interface ---- */

TEST_F(it_interface_type, struct_extends_interface_match) {
  vm_t vm = vm_create(allocator);

  /* create interface Printable with method: print(*const Point) -> void */
  value_t itv = vm_create_interface_type_value(vm, "Printable", true, "<builtin>");

  /* create a Point struct first to get its type */
  value_t stv = vm_create_struct_type_value(vm, "Point", true, "<builtin>");
  (void)vm_struct_add_field(vm, stv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_add_field(vm, stv, "y", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, stv);
  struct_type_t st = (struct_type_t)value_get_data(stv);

  /* pointer type: *const Point */
  value_t const_ptr_tv = vm_create_pointer_type_value(vm, (type_t)st, false, false);
  pointer_type_t const_ptr = (pointer_type_t)value_get_data(const_ptr_tv);

  /* callable_type: print(*const Point) -> void */
  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  vec_push(params, (type_t)const_ptr);
  callable_type_t iface_ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                                  "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);

  {
    value_t ctv = _make_callable_type_val(vm, iface_ct);
    vm_interface_add_method(vm, itv, "print", ctv);
  }
  vm_interface_seal(vm, itv);

  /* add matching method to struct (same callable_type) */
  callable_type_t struct_ct = (callable_type_t)alloc_clone(vm_get_allocator(vm), iface_ct);
  value_t fn = create_callable_value(vm, struct_ct, NULL, "print");
  (void)vm_struct_add_prop(vm, stv, "print", fn, true, true);

  /* check: struct implements interface */
  value_t ext = vm_interface_check_extends(vm, itv, vm_struct_get_methods(vm, stv));
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, struct_extends_interface_missing_method) {
  vm_t vm = vm_create(allocator);

  /* create interface with a method */
  value_t itv = vm_create_interface_type_value(vm, "Printable", true, "<builtin>");

  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                            "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);

  {
    value_t ctv = _make_callable_type_val(vm, ct);
    vm_interface_add_method(vm, itv, "print", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create struct with NO methods */
  value_t stv = vm_create_struct_type_value(vm, "Empty", true, "<builtin>");
  (void)vm_struct_add_field(vm, stv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, stv);

  /* check: struct does NOT implement interface (missing method) */
  value_t ext = vm_interface_check_extends(vm, itv, vm_struct_get_methods(vm, stv));
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- vm_interface_check_extends — union implements interface ---- */

TEST_F(it_interface_type, union_extends_interface_match) {
  vm_t vm = vm_create(allocator);

  /* create interface with method: inspect() -> void */
  value_t itv = vm_create_interface_type_value(vm, "Inspectable", true, "<builtin>");

  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t iface_ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                                  "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);

  {
    value_t ctv = _make_callable_type_val(vm, iface_ct);
    vm_interface_add_method(vm, itv, "inspect", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create union with matching method */
  value_t utv = vm_create_union_type_value(vm, "Option", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "some", ft, true);
  }
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "none", ft, true);
  }
  vm_union_seal(vm, utv);

  callable_type_t union_ct = (callable_type_t)alloc_clone(vm_get_allocator(vm), iface_ct);
  value_t fn = create_callable_value(vm, union_ct, NULL, "inspect");
  vm_union_add_prop(vm, utv, "inspect", fn, true, true);

  /* check: union implements interface */
  value_t ext = vm_interface_check_extends(vm, itv, vm_union_get_methods(vm, utv));
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, union_extends_interface_missing_method) {
  vm_t vm = vm_create(allocator);

  /* create interface with method */
  value_t itv = vm_create_interface_type_value(vm, "Inspectable", true, "<builtin>");

  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                            "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);

  {
    value_t ctv = _make_callable_type_val(vm, ct);
    vm_interface_add_method(vm, itv, "inspect", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create union with NO methods */
  value_t utv = vm_create_union_type_value(vm, "Simple", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "a", ft, true);
  }
  {
    value_t ft = _make_type_val(vm, _get_f64_type(vm));
    vm_union_add_field(vm, utv, "b", ft, true);
  }
  vm_union_seal(vm, utv);

  /* check: union does NOT implement interface */
  value_t ext = vm_interface_check_extends(vm, itv, vm_union_get_methods(vm, utv));
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- struct type_extends with interface ---- */

TEST_F(it_interface_type, struct_type_extends_interface_via_vtable) {
  vm_t vm = vm_create(allocator);

  /* create interface */
  value_t itv = vm_create_interface_type_value(vm, "Addable", true, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(itv);

  type_t i32_t = _get_i32_type(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t iface_ct = callable_type_create(vm_get_allocator(vm), params, i32_t, false, true,
                                                  "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);
  {
    value_t ctv = _make_callable_type_val(vm, iface_ct);
    vm_interface_add_method(vm, itv, "add", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create struct with matching method */
  value_t stv = vm_create_struct_type_value(vm, "Counter", true, "<builtin>");
  struct_type_t st = (struct_type_t)value_get_data(stv);
  (void)vm_struct_add_field(vm, stv, "val", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, stv);

  callable_type_t struct_ct = (callable_type_t)alloc_clone(vm_get_allocator(vm), iface_ct);
  value_t fn = create_callable_value(vm, struct_ct, NULL, "add");
  (void)vm_struct_add_prop(vm, stv, "add", fn, true, true);

  /* struct type_extends interface */
  vtable_t svt = type_get_vtable((type_t)st);
  ASSERT_TRUE(svt.type_extends != NULL);
  value_t ext = svt.type_extends(vm, (type_t)st, (type_t)it);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, struct_type_extends_interface_mismatch) {
  vm_t vm = vm_create(allocator);

  /* create interface with method signature: () -> i32 */
  value_t itv = vm_create_interface_type_value(vm, "Countable", true, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(itv);

  type_t i32_t = _get_i32_type(vm);
  type_t f64_t = _get_f64_type(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t iface_ct = callable_type_create(vm_get_allocator(vm), params, i32_t, false, true,
                                                  "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);
  {
    value_t ctv = _make_callable_type_val(vm, iface_ct);
    vm_interface_add_method(vm, itv, "count", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create struct with DIFFERENT method signature: () -> f64 */
  value_t stv = vm_create_struct_type_value(vm, "BadCounter", true, "<builtin>");
  struct_type_t st = (struct_type_t)value_get_data(stv);
  (void)vm_struct_add_field(vm, stv, "val", vm_get_f64_type(vm), true);
  (void)vm_struct_seal(vm, stv);

  vec_init_t vi2 = {.auto_dispose = false};
  vec_t params2 = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi2);
  callable_type_t struct_ct = callable_type_create(vm_get_allocator(vm), params2, f64_t, false, true,
                                                   "<builtin>");
  allocator_free(vm_get_allocator(vm), &params2);
  value_t fn = create_callable_value(vm, struct_ct, NULL, "count");
  (void)vm_struct_add_prop(vm, stv, "count", fn, true, true);

  /* struct type_extends interface should be false (signature mismatch) */
  vtable_t svt = type_get_vtable((type_t)st);
  value_t ext = svt.type_extends(vm, (type_t)st, (type_t)it);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- union type_extends with interface ---- */

TEST_F(it_interface_type, union_type_extends_interface_via_vtable) {
  vm_t vm = vm_create(allocator);

  /* create interface */
  value_t itv = vm_create_interface_type_value(vm, "Describable", true, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(itv);

  type_t void_t = (type_t)value_get_data(vm_get_void_type(vm));
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class, &vi);
  callable_type_t iface_ct = callable_type_create(vm_get_allocator(vm), params, void_t, false, true,
                                                  "<builtin>");
  allocator_free(vm_get_allocator(vm), &params);
  {
    value_t ctv = _make_callable_type_val(vm, iface_ct);
    vm_interface_add_method(vm, itv, "describe", ctv);
  }
  vm_interface_seal(vm, itv);

  /* create union with matching method */
  value_t utv = vm_create_union_type_value(vm, "Result", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "ok", ft, true);
  }
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "err", ft, true);
  }
  vm_union_seal(vm, utv);

  callable_type_t union_ct = (callable_type_t)alloc_clone(vm_get_allocator(vm), iface_ct);
  value_t fn = create_callable_value(vm, union_ct, NULL, "describe");
  vm_union_add_prop(vm, utv, "describe", fn, true, true);

  union_type_t ut = (union_type_t)value_get_data(utv);

  /* union type_extends interface */
  vtable_t uvt = type_get_vtable((type_t)ut);
  ASSERT_TRUE(uvt.type_extends != NULL);
  value_t ext = uvt.type_extends(vm, (type_t)ut, (type_t)it);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- wildcard extends ---- */

TEST_F(it_interface_type, struct_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t stv = vm_create_struct_type_value(vm, "S", true, "<builtin>");
  struct_type_t st = (struct_type_t)value_get_data(stv);
  (void)vm_struct_add_field(vm, stv, "x", vm_get_i32_type(vm), true);
  (void)vm_struct_seal(vm, stv);

  /* struct extends wildcard is always true */
  vtable_t svt = type_get_vtable((type_t)st);
  type_t wc_type = (type_t)value_get_data(vm_get_wildcard_type(vm));
  value_t ext = svt.type_extends(vm, (type_t)st, wc_type);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_interface_type, union_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t utv = vm_create_union_type_value(vm, "U", true, "<builtin>");
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "a", ft, true);
  }
  {
    value_t ft = _make_type_val(vm, _get_i32_type(vm));
    vm_union_add_field(vm, utv, "b", ft, true);
  }
  vm_union_seal(vm, utv);
  union_type_t ut = (union_type_t)value_get_data(utv);

  /* union extends wildcard is always true */
  vtable_t uvt = type_get_vtable((type_t)ut);
  type_t wc_type = (type_t)value_get_data(vm_get_wildcard_type(vm));
  value_t ext = uvt.type_extends(vm, (type_t)ut, wc_type);
  ASSERT_EQ(type_get_kind(value_get_type(ext)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- interface vtable is all NULL ---- */

TEST_F(it_interface_type, interface_vtable_is_null) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "I", true, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(tv);

  vtable_t vt = type_get_vtable((type_t)it);
  /* interface is compile-time only — no vtable entries */
  EXPECT_EQ(vt.type_equal, nullptr);
  EXPECT_EQ(vt.type_extends, nullptr);
  EXPECT_EQ(vt.to_string, nullptr);
  EXPECT_EQ(vt.safe_cast, nullptr);
  EXPECT_EQ(vt.get_field, nullptr);
  EXPECT_EQ(vt.set_field, nullptr);
  EXPECT_EQ(vt.member_call, nullptr);
  EXPECT_EQ(vt.get_prop, nullptr);
  EXPECT_EQ(vt.set_prop, nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- interface size and align are zero ---- */

TEST_F(it_interface_type, interface_size_align_zero) {
  vm_t vm = vm_create(allocator);
  value_t tv = vm_create_interface_type_value(vm, "I", true, "<builtin>");
  interface_type_t it = (interface_type_t)value_get_data(tv);

  EXPECT_EQ(type_get_size((type_t)it), 0u);
  EXPECT_EQ(type_get_align((type_t)it), 0u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
