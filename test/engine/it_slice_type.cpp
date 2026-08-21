#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/scope.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_slice_type : public CubecTest {
protected:

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }

  /* create array type via vm 鈥?registered in scope, no leak */
  array_type_t _make_i32_array_type(vm_t vm, uint64_t count) {
    value_t cv = create_i32_value(vm, (int32_t)count);
    value_t tv = vm_create_array_type_value(vm, _get_i32_type(vm), cv, true);
    return (array_type_t)value_get_data(tv);
  }

  /* create slice type via vm 鈥?registered in scope, no leak */
  slice_type_t _make_i32_slice_type(vm_t vm) {
    value_t tv = vm_create_slice_type_value(vm, _get_i32_type(vm), true);
    return (slice_type_t)value_get_data(tv);
  }

  slice_type_t _make_const_i32_slice_type(vm_t vm) {
    value_t tv = vm_create_slice_type_value(vm, _get_i32_type(vm), false);
    return (slice_type_t)value_get_data(tv);
  }

  /* helper: create a [3]i32 array value with [10, 20, 30] */
  value_t _make_i32_array_3(vm_t vm, array_type_t at) {
    int32_t v0 = 10, v1 = 20, v2 = 30;
    value_t elems[] = {
      vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
      vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
      vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
    };
    return create_array_value(vm, at, elems);
  }
};

/* ---- Type creation ---- */

TEST_F(it_slice_type, create_type) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  slice_type_t st = _make_i32_slice_type(vm);

  EXPECT_EQ(type_get_kind((type_t)st), TYPE_KIND_SLICE);
  EXPECT_STREQ(type_get_name((type_t)st), "[]i32");
  EXPECT_EQ(type_get_size((type_t)st), sizeof(struct slice_data_t));
  EXPECT_EQ(type_get_align((type_t)st), alignof(struct slice_data_t));
  EXPECT_TRUE(type_is_mut((type_t)st));
  EXPECT_EQ(type_get_kind(slice_type_get_element_type(st)), type_get_kind(i32t));

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, create_const_type) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_const_i32_slice_type(vm);

  EXPECT_FALSE(type_is_mut((type_t)st));

  vm_dispose(vm, allocator);
}

/* ---- Value creation ---- */

TEST_F(it_slice_type, create_value) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  EXPECT_NE(sl, nullptr);
  EXPECT_EQ(value_get_type(sl), (type_t)st);
  EXPECT_TRUE(value_is_initialized(sl));
  EXPECT_NE(value_get_data(sl), nullptr);

  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(sl);
  EXPECT_EQ(sd->start, 0u);
  EXPECT_EQ(sd->len, 3u);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, create_partial_slice) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 5);
  slice_type_t st = _make_i32_slice_type(vm);

  int32_t vals[] = {10, 20, 30, 40, 50};
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &vals[0], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[1], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[2], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[3], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[4], NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  /* slice [1..3] 鈫?elements at index 1 and 2 */
  value_t sl = create_slice_value(vm, st, arr, 1, 2);

  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(sl);
  EXPECT_EQ(sd->start, 1u * 4u); /* byte offset: 1 * sizeof(i32) */
  EXPECT_EQ(sd->len, 2u);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t sl = create_slice_shadow(vm, st, false);
  EXPECT_TRUE(value_is_shadow(sl));
  EXPECT_FALSE(value_is_initialized(sl));

  vm_dispose(vm, allocator);
}

/* ---- get_item ---- */

TEST_F(it_slice_type, get_item) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  value_t idx1 = create_i32_value(vm, 1);
  value_t elem = value_get_item(vm, sl, idx1);
  EXPECT_EQ(type_get_kind(value_get_type(elem)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(elem), 20);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, get_item_offset_slice) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 5);
  slice_type_t st = _make_i32_slice_type(vm);

  int32_t vals[] = {10, 20, 30, 40, 50};
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &vals[0], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[1], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[2], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[3], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[4], NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  /* slice [2..5) 鈫?{30, 40, 50} */
  value_t sl = create_slice_value(vm, st, arr, 2, 3);

  value_t idx0 = create_i32_value(vm, 0);
  value_t elem = value_get_item(vm, sl, idx0);
  EXPECT_EQ(*(int32_t *)value_get_data(elem), 30);

  value_t idx2 = create_i32_value(vm, 2);
  elem = value_get_item(vm, sl, idx2);
  EXPECT_EQ(*(int32_t *)value_get_data(elem), 50);

  vm_dispose(vm, allocator);
}

/* ---- set_item ---- */

TEST_F(it_slice_type, set_item) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  int32_t new_val = 99;
  value_t idx1 = create_i32_value(vm, 1);
  value_t val = vm_create_value(vm, _get_i32_type(vm), &new_val, NULL);
  value_set_item(vm, sl, idx1, val);

  /* read back from the slice */
  value_t got = value_get_item(vm, sl, idx1);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  /* also visible in the source array (slice borrows from array) */
  value_t arr_idx1 = create_i32_value(vm, 1);
  value_t arr_got = value_get_item(vm, arr, arr_idx1);
  EXPECT_EQ(*(int32_t *)value_get_data(arr_got), 99);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, set_item_const_error) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t const_st = _make_const_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, const_st, arr, 0, 3);

  int32_t new_val = 99;
  value_t idx0 = create_i32_value(vm, 0);
  value_t val = vm_create_value(vm, _get_i32_type(vm), &new_val, NULL);
  value_t result = value_set_item(vm, sl, idx0, val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- out of bounds ---- */

TEST_F(it_slice_type, out_of_bounds) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  value_t idx = create_i32_value(vm, 5);
  value_t result = value_get_item(vm, sl, idx);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, create_out_of_bounds) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);

  /* start_elem + count > array count */
  value_t sl = create_slice_value(vm, st, arr, 1, 3);
  EXPECT_EQ(type_get_kind(value_get_type(sl)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Equal ---- */

TEST_F(it_slice_type, equal_same) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl1 = create_slice_value(vm, st, arr, 0, 3);
  value_t sl2 = create_slice_value(vm, st, arr, 0, 3);

  value_t eq = value_equal(vm, sl1, sl2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, equal_different_len) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl1 = create_slice_value(vm, st, arr, 0, 3);
  value_t sl2 = create_slice_value(vm, st, arr, 0, 2);

  value_t eq = value_equal(vm, sl1, sl2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

/* ---- type_equal / type_extends ---- */

TEST_F(it_slice_type, type_equal_same) {
  vm_t vm = vm_create(allocator);
  slice_type_t st1 = _make_i32_slice_type(vm);
  slice_type_t st2 = _make_i32_slice_type(vm);

  vtable_t vt = type_get_vtable((type_t)st1);
  value_t eq = vt.type_equal(vm, (type_t)st1, (type_t)st2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  type_t wc = (type_t)value_get_data(vm_get_wildcard_type(vm));

  vtable_t vt = type_get_vtable((type_t)st);
  value_t ext = vt.type_extends(vm, (type_t)st, wc);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
}

/* ---- safe_cast ---- */

TEST_F(it_slice_type, safe_cast_identity) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  value_t cast = value_safe_cast(vm, sl, (type_t)st);
  EXPECT_EQ(cast, sl);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, safe_cast_mut_to_const) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t mut_st = _make_i32_slice_type(vm);
  slice_type_t const_st = _make_const_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, mut_st, arr, 0, 3);

  value_t cast = value_safe_cast(vm, sl, (type_t)const_st);
  EXPECT_NE(cast, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_SLICE);
  EXPECT_FALSE(type_is_mut(value_get_type(cast)));

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, safe_cast_const_to_mut_error) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t mut_st = _make_i32_slice_type(vm);
  slice_type_t const_st = _make_const_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, const_st, arr, 0, 3);

  value_t cast = value_safe_cast(vm, sl, (type_t)mut_st);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Assignment ---- */

TEST_F(it_slice_type, assignment) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t dst = create_slice_value(vm, st, arr, 0, 2);
  value_t src = create_slice_value(vm, st, arr, 1, 2);

  value_t result = value_assignment(vm, dst, src);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(dst);
  EXPECT_EQ(sd->start, 1u * 4u);
  EXPECT_EQ(sd->len, 2u);

  vm_dispose(vm, allocator);
}

/* ---- deref_get ---- */

TEST_F(it_slice_type, deref_get) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 1, 2);

  /* deref_get returns first element of slice 鈫?arr[1] = 20 */
  value_t first = value_deref_get(vm, sl);
  EXPECT_EQ(type_get_kind(value_get_type(first)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(first), 20);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, deref_get_empty_error) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 0);

  value_t result = value_deref_get(vm, sl);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- to_string ---- */

TEST_F(it_slice_type, to_string) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  value_t s = value_to_string(vm, sl);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(s)), "[10, 20, 30]");

  vm_dispose(vm, allocator);
}

/* ---- Clone ---- */

TEST_F(it_slice_type, clone) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  value_t cloned = value_clone(vm, sl);
  EXPECT_NE(cloned, sl);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_SLICE);
  EXPECT_TRUE(value_is_initialized(cloned));

  /* cloned slice_data_t is shallow copy (same ptr/start/len) */
  struct slice_data_t *src_sd = (struct slice_data_t *)value_get_data(sl);
  struct slice_data_t *dst_sd = (struct slice_data_t *)value_get_data(cloned);
  EXPECT_EQ(dst_sd->start, src_sd->start);
  EXPECT_EQ(dst_sd->len, src_sd->len);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_clone) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t sl = create_slice_shadow(vm, st, false);
  value_t cloned = value_clone(vm, sl);

  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_SLICE);

  vm_dispose(vm, allocator);
}

/* ---- Cross-scope clone ---- */

TEST_F(it_slice_type, cross_scope_clone) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  array_type_t at = _make_i32_array_type(vm, 3);
  slice_type_t st = _make_i32_slice_type(vm);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = create_slice_value(vm, st, arr, 0, 3);

  /* switch to callee scope */
  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t local = value_clone(vm, sl);
  EXPECT_EQ(type_get_kind(value_get_type(local)), TYPE_KIND_SLICE);

  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(local);
  EXPECT_EQ(sd->len, 3u);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &callee);

  vm_dispose(vm, allocator);
}

/* ---- type_clone ---- */

TEST_F(it_slice_type, type_clone_cross_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t i32t = _get_i32_type(vm);

  /* create slice type in outer scope */
  value_t tv = vm_create_slice_type_value(vm, i32t, true);
  slice_type_t outer_st = (slice_type_t)value_get_data(tv);

  /* switch to inner scope */
  scope_t inner = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, inner);
  scope_t prev_root = vm_set_root_scope(vm, inner);

  /* types are global singletons (vm->types) — same pointer, not cloned */
  type_t inner_type = (type_t)outer_st;
  slice_type_t inner_st = (slice_type_t)inner_type;
  EXPECT_EQ(inner_type, (type_t)outer_st);
  EXPECT_EQ(type_get_kind(inner_type), TYPE_KIND_SLICE);
  EXPECT_EQ(type_get_kind(slice_type_get_element_type(inner_st)), TYPE_KIND_I32);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &inner);

  vm_dispose(vm, allocator);
}

/* ---- vm_create_slice_type_value registers in scope ---- */

TEST_F(it_slice_type, vm_create_slice_type_value_registers_in_scope) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);

  scope_t scope = vm_get_current_scope(vm);
  size_t types_before = vec_get_size(vm_get_types(vm));

  value_t tv = vm_create_slice_type_value(vm, i32t, true);
  EXPECT_NE(tv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(tv)), TYPE_KIND_TYPE);

  slice_type_t st = (slice_type_t)value_get_data(tv);
  EXPECT_EQ(type_get_kind((type_t)st), TYPE_KIND_SLICE);

  /* registered in vm->types */
  EXPECT_EQ(vec_get_size(vm_get_types(vm)), types_before + 1);

  vm_dispose(vm, allocator);
}

/* ---- create_slice_value from non-array error ---- */

TEST_F(it_slice_type, create_from_non_array_error) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);

  /* i32 value is not an array */
  int32_t val = 42;
  value_t not_array = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t sl = create_slice_value(vm, st, not_array, 0, 1);
  EXPECT_EQ(type_get_kind(value_get_type(sl)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- value_slice on array ---- */

TEST_F(it_slice_type, array_slice) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 5);

  int32_t vals[] = {10, 20, 30, 40, 50};
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &vals[0], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[1], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[2], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[3], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[4], NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t sl = value_slice(vm, arr, 1, 3);
  EXPECT_EQ(type_get_kind(value_get_type(sl)), TYPE_KIND_SLICE);

  /* slice[0] = 20, slice[1] = 30, slice[2] = 40 */
  value_t idx0 = create_i32_value(vm, 0);
  value_t idx1 = create_i32_value(vm, 1);
  value_t idx2 = create_i32_value(vm, 2);
  EXPECT_EQ(*(int32_t *)value_get_data(value_get_item(vm, sl, idx0)), 20);
  EXPECT_EQ(*(int32_t *)value_get_data(value_get_item(vm, sl, idx1)), 30);
  EXPECT_EQ(*(int32_t *)value_get_data(value_get_item(vm, sl, idx2)), 40);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, array_slice_out_of_bounds) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 3);

  value_t arr = _make_i32_array_3(vm, at);
  value_t sl = value_slice(vm, arr, 1, 3);
  EXPECT_EQ(type_get_kind(value_get_type(sl)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- value_slice on slice (slice of slice) ---- */

TEST_F(it_slice_type, slice_of_slice) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 5);
  slice_type_t st = _make_i32_slice_type(vm);

  int32_t vals[] = {10, 20, 30, 40, 50};
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &vals[0], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[1], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[2], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[3], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[4], NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  /* first slice: [1..4) 鈫?{20, 30, 40} */
  value_t sl1 = create_slice_value(vm, st, arr, 1, 3);

  /* second slice: sl1[1..3) 鈫?{30, 40} */
  value_t sl2 = value_slice(vm, sl1, 1, 2);
  EXPECT_EQ(type_get_kind(value_get_type(sl2)), TYPE_KIND_SLICE);

  value_t idx0 = create_i32_value(vm, 0);
  value_t idx1 = create_i32_value(vm, 1);
  EXPECT_EQ(*(int32_t *)value_get_data(value_get_item(vm, sl2, idx0)), 30);
  EXPECT_EQ(*(int32_t *)value_get_data(value_get_item(vm, sl2, idx1)), 40);

  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, slice_of_slice_out_of_bounds) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array_type(vm, 5);
  slice_type_t st = _make_i32_slice_type(vm);

  int32_t vals[] = {10, 20, 30, 40, 50};
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &vals[0], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[1], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[2], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[3], NULL),
    vm_create_value(vm, _get_i32_type(vm), &vals[4], NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  value_t sl1 = create_slice_value(vm, st, arr, 1, 3);
  /* sl1 has len=3, so start=2 count=2 is out of bounds */
  value_t sl2 = value_slice(vm, sl1, 2, 2);
  EXPECT_EQ(type_get_kind(value_get_type(sl2)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Shadow operations ---- */

TEST_F(it_slice_type, shadow_equal) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t a = create_slice_shadow(vm, st, true);
  value_t b = create_slice_shadow(vm, st, true);
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_assignment) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t a = create_slice_shadow(vm, st, false);
  value_t b = create_slice_shadow(vm, st, true);
  value_t result = value_assignment(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));
  EXPECT_TRUE(value_is_shadow(a));
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_get_item) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t sl = create_slice_shadow(vm, st, true);
  value_t idx = create_i32_value(vm, 0);
  value_t result = value_get_item(vm, sl, idx);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_set_item) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t sl = create_slice_shadow(vm, st, false);
  value_t idx = create_i32_value(vm, 0);
  value_t val = create_i32_value(vm, 42);
  value_t result = value_set_item(vm, sl, idx, val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(sl));
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_deref_get) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t sl = create_slice_shadow(vm, st, true);
  value_t result = value_deref_get(vm, sl);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_safe_cast) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  slice_type_t const_st = _make_const_i32_slice_type(vm);
  value_t sl = create_slice_shadow(vm, st, true);
  value_t result = value_safe_cast(vm, sl, (type_t)const_st);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(value_get_type(result), (type_t)const_st);
  vm_dispose(vm, allocator);
}

TEST_F(it_slice_type, shadow_to_string) {
  vm_t vm = vm_create(allocator);
  slice_type_t st = _make_i32_slice_type(vm);
  value_t sl = create_slice_shadow(vm, st, true);
  value_t result = value_to_string(vm, sl);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}
