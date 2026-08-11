#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
#include "engine/array_type.h"
#include "engine/scope.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_array_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }

  /* create array type via vm — registered in scope, no leak */
  array_type_t _make_i32_array_type(vm_t vm, uint64_t count) {
    value_t tv = vm_create_array_type_value(vm, _get_i32_type(vm), count, true);
    return (array_type_t)value_get_data(tv);
  }

  array_type_t _make_const_i32_array_type(vm_t vm, uint64_t count) {
    value_t tv = vm_create_array_type_value(vm, _get_i32_type(vm), count, false);
    return (array_type_t)value_get_data(tv);
  }
};

/* ---- Type creation ---- */

TEST_F(it_array_type, create_type) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  array_type_t at = _make_i32_array_type(vm, 3);

  EXPECT_EQ(type_get_kind((type_t)at), TYPE_KIND_ARRAY);
  EXPECT_STREQ(type_get_name((type_t)at), "[3]i32");
  EXPECT_EQ(type_get_size((type_t)at), 3u * 4u);
  EXPECT_EQ(type_get_align((type_t)at), 4u);
  EXPECT_TRUE(type_is_mut((type_t)at));
  EXPECT_EQ(array_type_get_element_type(at), i32t);
  EXPECT_EQ(array_type_get_count(at), 3u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, create_const_type) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_const_i32_array_type(vm, 5);

  EXPECT_FALSE(type_is_mut((type_t)at));
  EXPECT_EQ(array_type_get_count(at), 5u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, create_empty_type) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 0);

  EXPECT_EQ(type_get_size((type_t)at), 0u);
  EXPECT_EQ(array_type_get_count(at), 0u);
  EXPECT_STREQ(type_get_name((type_t)at), "[0]i32");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Value creation ---- */

TEST_F(it_array_type, create_value) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  int32_t v0 = 10, v1 = 20, v2 = 30;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };

  value_t arr = create_array_value(vm, at, elems);
  EXPECT_NE(arr, nullptr);
  EXPECT_EQ(value_get_type(arr), (type_t)at);
  EXPECT_TRUE(value_is_own(arr));
  EXPECT_TRUE(value_is_initialized(arr));
  EXPECT_NE(value_get_data(arr), nullptr);

  int32_t *data = (int32_t *)value_get_data(arr);
  EXPECT_EQ(data[0], 10);
  EXPECT_EQ(data[1], 20);
  EXPECT_EQ(data[2], 30);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  value_t arr = create_array_shadow(vm, at, false);
  EXPECT_TRUE(value_is_shadow(arr));
  EXPECT_FALSE(value_is_initialized(arr));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Clone ---- */

TEST_F(it_array_type, clone) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  int32_t v0 = 1, v1 = 2, v2 = 3;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t cloned = value_clone(vm, arr);
  EXPECT_NE(cloned, arr);
  /* type is cloned into current scope — may be different pointer */
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_ARRAY);
  EXPECT_TRUE(value_is_own(cloned));

  int32_t *src_data = (int32_t *)value_get_data(arr);
  int32_t *dst_data = (int32_t *)value_get_data(cloned);
  EXPECT_NE(dst_data, src_data);
  EXPECT_EQ(dst_data[0], 1);
  EXPECT_EQ(dst_data[1], 2);
  EXPECT_EQ(dst_data[2], 3);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, clone_independence) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t v0 = 10, v1 = 20;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);
  value_t cloned = value_clone(vm, arr);

  ((int32_t *)value_get_data(arr))[0] = 99;
  EXPECT_EQ(((int32_t *)value_get_data(cloned))[0], 10);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- get_item / set_item ---- */

TEST_F(it_array_type, get_item) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  int32_t v0 = 10, v1 = 20, v2 = 30;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t idx1 = create_i32_value(vm, 1);
  value_t elem = value_get_item(vm, arr, idx1);
  EXPECT_EQ(type_get_kind(value_get_type(elem)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(elem), 20);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, set_item) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  int32_t v0 = 10, v1 = 20, v2 = 30;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  int32_t new_val = 99;
  value_t idx0 = create_i32_value(vm, 0);
  value_t val = vm_create_value(vm, _get_i32_type(vm), &new_val, NULL);
  value_set_item(vm, arr, idx0, val);

  value_t got = value_get_item(vm, arr, idx0);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, out_of_bounds) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t v0 = 1, v1 = 2;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t idx = create_i32_value(vm, 5);
  value_t result = value_get_item(vm, arr, idx);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Equal ---- */

TEST_F(it_array_type, equal_same) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t a0 = 1, a1 = 2;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &a1, NULL),
  };
  value_t arr_a = create_array_value(vm, at, elems_a);

  int32_t b0 = 1, b1 = 2;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b1, NULL),
  };
  value_t arr_b = create_array_value(vm, at, elems_b);

  value_t eq = value_equal(vm, arr_a, arr_b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, equal_different) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t a0 = 1, a1 = 2;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &a1, NULL),
  };
  value_t arr_a = create_array_value(vm, at, elems_a);

  int32_t b0 = 1, b1 = 99;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b1, NULL),
  };
  value_t arr_b = create_array_value(vm, at, elems_b);

  value_t eq = value_equal(vm, arr_a, arr_b);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_equal / type_extends ---- */

TEST_F(it_array_type, type_equal_same) {
  vm_t vm = vm_create(allocator);
  array_type_t at1 = _make_i32_array_type(vm, 3);
  array_type_t at2 = _make_i32_array_type(vm, 3);

  vtable_t vt = type_get_vtable((type_t)at1);
  value_t eq = vt.type_equal(vm, (type_t)at1, (type_t)at2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, type_equal_different_count) {
  vm_t vm = vm_create(allocator);
  array_type_t at1 = _make_i32_array_type(vm, 3);
  array_type_t at2 = _make_i32_array_type(vm, 5);

  vtable_t vt = type_get_vtable((type_t)at1);
  value_t eq = vt.type_equal(vm, (type_t)at1, (type_t)at2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  type_t wc = (type_t)value_get_data(vm_get_wildcard_type(vm));

  vtable_t vt = type_get_vtable((type_t)at);
  value_t ext = vt.type_extends(vm, (type_t)at, wc);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast ---- */

TEST_F(it_array_type, safe_cast_identity) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t v0 = 1, v1 = 2;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t cast = value_safe_cast(vm, arr, (type_t)at);
  EXPECT_EQ(cast, arr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, safe_cast_mut_to_const) {
  vm_t vm = vm_create(allocator);
  array_type_t mut_at = _make_i32_array_type(vm, 2);
  array_type_t const_at = _make_const_i32_array_type(vm, 2);

  int32_t v0 = 1, v1 = 2;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, mut_at, elems);

  value_t cast = value_safe_cast(vm, arr, (type_t)const_at);
  EXPECT_NE(cast, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ARRAY);
  EXPECT_FALSE(type_is_mut(value_get_type(cast)));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, safe_cast_const_to_mut_error) {
  vm_t vm = vm_create(allocator);
  array_type_t mut_at = _make_i32_array_type(vm, 2);
  array_type_t const_at = _make_const_i32_array_type(vm, 2);

  int32_t v0 = 1, v1 = 2;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, const_at, elems);

  value_t cast = value_safe_cast(vm, arr, (type_t)mut_at);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Assignment ---- */

TEST_F(it_array_type, assignment) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t a0 = 1, a1 = 2;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &a1, NULL),
  };
  value_t dst = create_array_value(vm, at, elems_a);

  int32_t b0 = 10, b1 = 20;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b1, NULL),
  };
  value_t src = create_array_value(vm, at, elems_b);

  value_t result = value_assignment(vm, dst, src);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  int32_t *data = (int32_t *)value_get_data(dst);
  EXPECT_EQ(data[0], 10);
  EXPECT_EQ(data[1], 20);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- to_string ---- */

TEST_F(it_array_type, to_string) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  int32_t v0 = 1, v1 = 2, v2 = 3;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t s = value_to_string(vm, arr);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(s)), "[1, 2, 3]");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-scope clone ---- */

TEST_F(it_array_type, cross_scope_clone) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  array_type_t at = _make_i32_array_type(vm, 2);

  int32_t v0 = 10, v1 = 20;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
    vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t local = value_clone(vm, arr);
  int32_t *ldata = (int32_t *)value_get_data(local);
  EXPECT_EQ(ldata[0], 10);
  EXPECT_EQ(ldata[1], 20);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);

  int32_t *odata = (int32_t *)value_get_data(arr);
  EXPECT_EQ(odata[0], 10);

  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Empty array ---- */

TEST_F(it_array_type, empty_array) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 0);
  value_t arr = create_array_value(vm, at, NULL);

  EXPECT_NE(arr, nullptr);
  EXPECT_TRUE(value_is_initialized(arr));
  EXPECT_EQ(type_get_size((type_t)at), 0u);

  value_t idx = create_i32_value(vm, 0);
  value_t result = value_get_item(vm, arr, idx);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Shadow clone ---- */

TEST_F(it_array_type, shadow_clone) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  value_t arr = create_array_shadow(vm, at, false);
  value_t cloned = value_clone(vm, arr);

  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_EQ(value_get_type(cloned), (type_t)at);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- vm_create_array_type_value ---- */

TEST_F(it_array_type, vm_create_array_type_value_registers_in_scope) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);

  scope_t scope = vm_get_current_scope(vm);
  size_t types_before = vec_get_size(scope->types);

  value_t tv = vm_create_array_type_value(vm, i32t, 4, true);
  EXPECT_NE(tv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(tv)), TYPE_KIND_TYPE);

  array_type_t at = (array_type_t)value_get_data(tv);
  EXPECT_EQ(array_type_get_count(at), 4u);

  /* registered in scope->types */
  EXPECT_EQ(vec_get_size(scope->types), types_before + 1);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type_clone: recursive for nested arrays ---- */

TEST_F(it_array_type, type_clone_same_scope) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);

  value_t tv = vm_create_array_type_value(vm, i32t, 3, true);
  array_type_t at = (array_type_t)value_get_data(tv);

  /* type_clone in same scope returns same pointer (already in scope) */
  type_t cloned = value_type_clone(vm, (type_t)at);
  /* for static element types, same-scope clone reuses existing type */
  EXPECT_EQ(type_get_kind(cloned), TYPE_KIND_ARRAY);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, type_clone_cross_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t i32t = _get_i32_type(vm);

  /* create array type in outer scope */
  value_t tv = vm_create_array_type_value(vm, i32t, 3, true);
  array_type_t outer_at = (array_type_t)value_get_data(tv);

  /* switch to inner scope */
  scope_t inner = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, inner);
  scope_t prev_root = vm_set_root_scope(vm, inner);

  /* type_clone into inner scope */
  type_t inner_type = value_type_clone(vm, (type_t)outer_at);
  array_type_t inner_at = (array_type_t)inner_type;
  EXPECT_NE(inner_type, (type_t)outer_at);
  EXPECT_EQ(type_get_kind(inner_type), TYPE_KIND_ARRAY);
  EXPECT_EQ(array_type_get_count(inner_at), 3u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(inner_at)), TYPE_KIND_I32);

  /* inner scope owns the cloned type */
  EXPECT_GT(vec_get_size(inner->types), 0u);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &inner);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_array_type, nested_array_cross_scope_clone) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t i32t = _get_i32_type(vm);

  /* create [2][3]i32 in outer scope */
  value_t inner_tv = vm_create_array_type_value(vm, i32t, 3, true);
  array_type_t inner_at = (array_type_t)value_get_data(inner_tv);

  value_t outer_tv = vm_create_array_type_value(vm, (type_t)inner_at, 2, true);
  array_type_t outer_at = (array_type_t)value_get_data(outer_tv);

  /* create [2][3]i32 value: [[1,2,3],[4,5,6]] */
  int32_t row0[] = {1, 2, 3};
  int32_t row1[] = {4, 5, 6};
  value_t rows[] = {
    vm_create_value(vm, (type_t)inner_at, row0, NULL),
    vm_create_value(vm, (type_t)inner_at, row1, NULL),
  };
  value_t arr = create_array_value(vm, outer_at, rows);

  /* switch to callee scope */
  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  /* clone: should recursively clone [2][3]i32 type and data */
  value_t cloned = value_clone(vm, arr);
  EXPECT_NE(cloned, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_ARRAY);

  array_type_t cloned_outer = (array_type_t)value_get_type(cloned);
  EXPECT_EQ(array_type_get_count(cloned_outer), 2u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(cloned_outer)), TYPE_KIND_ARRAY);

  array_type_t cloned_inner = (array_type_t)array_type_get_element_type(cloned_outer);
  EXPECT_EQ(array_type_get_count(cloned_inner), 3u);

  /* verify data: get cloned[0][1] should be 2 */
  value_t idx0 = create_i32_value(vm, 0);
  value_t first_row = value_get_item(vm, cloned, idx0);
  value_t idx1 = create_i32_value(vm, 1);
  value_t elem = value_get_item(vm, first_row, idx1);
  EXPECT_EQ(*(int32_t *)value_get_data(elem), 2);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &callee);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
