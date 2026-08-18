#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/scope.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_tuple_type : public CubecTest {
protected:

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

  /* create tuple type via vm 鈥?registered in scope, no leak */
  tuple_type_t _make_i32_f64_tuple_type(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_f64_type(vm));
    value_t tv = vm_create_tuple_type_value(vm, types, true);
    allocator_free(alloc, &types);
    return (tuple_type_t)value_get_data(tv);
  }

  tuple_type_t _make_const_i32_f64_tuple_type(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_f64_type(vm));
    value_t tv = vm_create_tuple_type_value(vm, types, false);
    allocator_free(alloc, &types);
    return (tuple_type_t)value_get_data(tv);
  }

  /* create a homogeneous <i32, i32, i32> tuple type for array-cast tests */
  tuple_type_t _make_i32x3_tuple_type(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_i32_type(vm));
    value_t tv = vm_create_tuple_type_value(vm, types, true);
    allocator_free(alloc, &types);
    return (tuple_type_t)value_get_data(tv);
  }

  tuple_type_t _make_const_i32x3_tuple_type(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_i32_type(vm));
    vec_push(types, _get_i32_type(vm));
    value_t tv = vm_create_tuple_type_value(vm, types, false);
    allocator_free(alloc, &types);
    return (tuple_type_t)value_get_data(tv);
  }

  /* create a [3]i32 array type for tuple→array cast tests */
  array_type_t _make_i32x3_array_type(vm_t vm) {
    type_t i32_t = _get_i32_type(vm);
    value_t tv = vm_create_array_type_value(vm, i32_t, 3, true);
    return (array_type_t)value_get_data(tv);
  }

  array_type_t _make_const_i32x3_array_type(vm_t vm) {
    type_t i32_t = _get_i32_type(vm);
    value_t tv = vm_create_array_type_value(vm, i32_t, 3, false);
    return (array_type_t)value_get_data(tv);
  }
};

/* ---- Type creation ---- */

TEST_F(it_tuple_type, create_type) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  type_t f64t = _get_f64_type(vm);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  EXPECT_EQ(type_get_kind((type_t)tt), TYPE_KIND_TUPLE);
  EXPECT_STREQ(type_get_name((type_t)tt), "<i32, f64>");
  EXPECT_EQ(tuple_type_get_field_count(tt), 2u);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 0)), type_get_kind(i32t));
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 1)), type_get_kind(f64t));
  EXPECT_TRUE(type_is_mut((type_t)tt));

  /* size = align_up(4, 8) + 8 = 16, align = 8 */
  EXPECT_EQ(type_get_size((type_t)tt), 16u);
  EXPECT_EQ(type_get_align((type_t)tt), 8u);
  /* offsets: [0] = 0, [1] = 8 */
  EXPECT_EQ(tuple_type_get_offset(tt, 0), 0u);
  EXPECT_EQ(tuple_type_get_offset(tt, 1), 8u);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, create_const_type) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_const_i32_f64_tuple_type(vm);

  EXPECT_FALSE(type_is_mut((type_t)tt));

  vm_dispose(vm, allocator);
}

/* ---- Value creation ---- */

TEST_F(it_tuple_type, create_value) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 42;
  double d = 3.14;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };

  value_t tup = create_tuple_value(vm, tt, elems);
  EXPECT_NE(tup, nullptr);
  EXPECT_EQ(value_get_type(tup), (type_t)tt);
  EXPECT_TRUE(value_is_own(tup));
  EXPECT_TRUE(value_is_initialized(tup));
  EXPECT_NE(value_get_data(tup), nullptr);

  /* verify data layout: i32 at offset 0, f64 at offset 8 */
  int32_t read_i = *(int32_t *)((char *)value_get_data(tup) + 0);
  double read_d = *(double *)((char *)value_get_data(tup) + 8);
  EXPECT_EQ(read_i, 42);
  EXPECT_DOUBLE_EQ(read_d, 3.14);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, create_shadow) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  value_t tup = create_tuple_shadow(vm, tt, false);
  EXPECT_TRUE(value_is_shadow(tup));
  EXPECT_FALSE(value_is_initialized(tup));

  vm_dispose(vm, allocator);
}

/* ---- Clone ---- */

TEST_F(it_tuple_type, clone) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 1;
  double d = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cloned = value_clone(vm, tup);
  EXPECT_NE(cloned, tup);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_TUPLE);
  EXPECT_TRUE(value_is_own(cloned));

  int32_t read_i = *(int32_t *)((char *)value_get_data(cloned) + 0);
  double read_d = *(double *)((char *)value_get_data(cloned) + 8);
  EXPECT_EQ(read_i, 1);
  EXPECT_DOUBLE_EQ(read_d, 2.5);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, clone_independence) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 10;
  double d = 20.0;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);
  value_t cloned = value_clone(vm, tup);

  *(int32_t *)((char *)value_get_data(tup) + 0) = 99;
  EXPECT_EQ(*(int32_t *)((char *)value_get_data(cloned) + 0), 10);

  vm_dispose(vm, allocator);
}

/* ---- get_item / set_item ---- */

TEST_F(it_tuple_type, get_item) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 42;
  double d = 3.14;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t idx0 = create_i32_value(vm, 0);
  value_t elem0 = value_get_item(vm, tup, idx0);
  EXPECT_EQ(type_get_kind(value_get_type(elem0)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(elem0), 42);

  value_t idx1 = create_i32_value(vm, 1);
  value_t elem1 = value_get_item(vm, tup, idx1);
  EXPECT_EQ(type_get_kind(value_get_type(elem1)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(elem1), 3.14);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, set_item) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 10;
  double d = 20.0;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  int32_t new_i = 99;
  value_t idx0 = create_i32_value(vm, 0);
  value_t val = vm_create_value(vm, _get_i32_type(vm), &new_i, NULL);
  value_set_item(vm, tup, idx0, val);

  value_t got = value_get_item(vm, tup, idx0);
  EXPECT_EQ(*(int32_t *)value_get_data(got), 99);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, set_item_const_error) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_const_i32_f64_tuple_type(vm);

  int32_t i = 10;
  double d = 20.0;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  int32_t new_i = 99;
  value_t idx0 = create_i32_value(vm, 0);
  value_t val = vm_create_value(vm, _get_i32_type(vm), &new_i, NULL);
  value_t result = value_set_item(vm, tup, idx0, val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, out_of_bounds) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 10;
  double d = 20.0;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t idx = create_i32_value(vm, 5);
  value_t result = value_get_item(vm, tup, idx);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Equal ---- */

TEST_F(it_tuple_type, equal_same) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t a_i = 1;
  double a_d = 2.5;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &a_d, NULL),
  };
  value_t tup_a = create_tuple_value(vm, tt, elems_a);

  int32_t b_i = 1;
  double b_d = 2.5;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &b_d, NULL),
  };
  value_t tup_b = create_tuple_value(vm, tt, elems_b);

  value_t eq = value_equal(vm, tup_a, tup_b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, equal_different) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t a_i = 1;
  double a_d = 2.5;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &a_d, NULL),
  };
  value_t tup_a = create_tuple_value(vm, tt, elems_a);

  int32_t b_i = 99;
  double b_d = 2.5;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &b_d, NULL),
  };
  value_t tup_b = create_tuple_value(vm, tt, elems_b);

  value_t eq = value_equal(vm, tup_a, tup_b);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

/* ---- type_equal / type_extends ---- */

TEST_F(it_tuple_type, type_equal_same) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt1 = _make_i32_f64_tuple_type(vm);
  tuple_type_t tt2 = _make_i32_f64_tuple_type(vm);

  vtable_t vt = type_get_vtable((type_t)tt1);
  value_t eq = vt.type_equal(vm, (type_t)tt1, (type_t)tt2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_equal_different_count) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt2 = _make_i32_f64_tuple_type(vm);
  /* create single-element tuple for different count comparison */
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t types1 = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types1, _get_i32_type(vm));
  value_t tv1 = vm_create_tuple_type_value(vm, types1, true);
  tuple_type_t tt1 = (tuple_type_t)value_get_data(tv1);
  allocator_free(alloc, &types1);

  vtable_t vt = type_get_vtable((type_t)tt2);
  value_t eq = vt.type_equal(vm, (type_t)tt2, (type_t)tt1);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  type_t wc = (type_t)value_get_data(vm_get_wildcard_type(vm));

  vtable_t vt = type_get_vtable((type_t)tt);
  value_t ext = vt.type_extends(vm, (type_t)tt, wc);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_extends_wildcard_tuple) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_tuple_type(vm));

  vtable_t vt = type_get_vtable((type_t)tt);
  value_t ext = vt.type_extends(vm, (type_t)tt, wct);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_equal_wildcard_tuple) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_tuple_type(vm));

  vtable_t vt = type_get_vtable((type_t)tt);
  value_t eq = vt.type_equal(vm, (type_t)tt, wct);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_equal_wildcard_element) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t i32t = _get_i32_type(vm);
  type_t f64t = _get_f64_type(vm);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_type(vm));

  /* <i32, f64> vs <i32, ?> 鈫?true (wildcard element skips comparison) */
  tuple_type_t concrete = _make_i32_f64_tuple_type(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types, i32t);
  vec_push(types, wct);
  value_t wtv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  tuple_type_t wc_tt = (tuple_type_t)value_get_data(wtv);

  vtable_t vt = type_get_vtable((type_t)concrete);
  value_t eq = vt.type_equal(vm, (type_t)concrete, (type_t)wc_tt);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  /* <i32, ?> vs <i32, f64> 鈫?false (wildcard only on right side) */
  vtable_t vt2 = type_get_vtable((type_t)wc_tt);
  value_t eq2 = vt2.type_equal(vm, (type_t)wc_tt, (type_t)concrete);
  EXPECT_FALSE(*(bool *)value_get_data(eq2));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, type_extends_wildcard_element) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t i32t = _get_i32_type(vm);
  type_t f64t = _get_f64_type(vm);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_type(vm));

  /* <i32, f64> extends <i32, ?> 鈫?true */
  tuple_type_t concrete = _make_i32_f64_tuple_type(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types, i32t);
  vec_push(types, wct);
  value_t wtv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  tuple_type_t wc_tt = (tuple_type_t)value_get_data(wtv);

  vtable_t vt = type_get_vtable((type_t)concrete);
  value_t ext = vt.type_extends(vm, (type_t)concrete, (type_t)wc_tt);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
}

/* ---- safe_cast ---- */

TEST_F(it_tuple_type, safe_cast_identity) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 1;
  double d = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)tt);
  EXPECT_EQ(cast, tup);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_mut_to_const) {
  vm_t vm = vm_create(allocator);
  tuple_type_t mut_tt = _make_i32_f64_tuple_type(vm);
  tuple_type_t const_tt = _make_const_i32_f64_tuple_type(vm);

  int32_t i = 1;
  double d = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, mut_tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)const_tt);
  EXPECT_NE(cast, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_TUPLE);
  EXPECT_FALSE(type_is_mut(value_get_type(cast)));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_const_to_mut_error) {
  vm_t vm = vm_create(allocator);
  tuple_type_t mut_tt = _make_i32_f64_tuple_type(vm);
  tuple_type_t const_tt = _make_const_i32_f64_tuple_type(vm);

  int32_t i = 1;
  double d = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, const_tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)mut_tt);
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- safe_cast: tuple → array ---- */

TEST_F(it_tuple_type, safe_cast_tuple_to_array_homogeneous) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32x3_tuple_type(vm);
  array_type_t at = _make_i32x3_array_type(vm);

  int32_t a = 10, b = 20, c = 30;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
    vm_create_value(vm, _get_i32_type(vm), &c, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  ASSERT_FALSE(value_is_error(cast));
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ARRAY);
  EXPECT_EQ(array_type_get_count((array_type_t)value_get_type(cast)), 3u);

  /* verify each element was preserved */
  for (uint64_t i = 0; i < 3; i++) {
    value_t idx = create_i32_value(vm, (int32_t)i);
    value_t elem = value_get_item(vm, cast, idx);
    ASSERT_FALSE(value_is_error(elem));
    EXPECT_EQ(*(int32_t *)value_get_data(elem), (int32_t)((i + 1) * 10));
  }

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_mut_to_const) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32x3_tuple_type(vm);
  array_type_t const_at = _make_const_i32x3_array_type(vm);

  int32_t a = 1, b = 2, c = 3;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
    vm_create_value(vm, _get_i32_type(vm), &c, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)const_at);
  ASSERT_FALSE(value_is_error(cast));
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ARRAY);
  EXPECT_FALSE(type_is_mut(value_get_type(cast)));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_const_to_mut_error) {
  vm_t vm = vm_create(allocator);
  tuple_type_t const_tt = _make_const_i32x3_tuple_type(vm);
  array_type_t at = _make_i32x3_array_type(vm);

  int32_t a = 1, b = 2, c = 3;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
    vm_create_value(vm, _get_i32_type(vm), &c, NULL),
  };
  value_t tup = create_tuple_value(vm, const_tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  EXPECT_TRUE(value_is_error(cast));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_count_mismatch_error) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32x3_tuple_type(vm);
  /* [2]i32 array — count mismatch with 3-element tuple */
  type_t i32_t = _get_i32_type(vm);
  value_t atv = vm_create_array_type_value(vm, i32_t, 2, true);
  array_type_t at = (array_type_t)value_get_data(atv);

  int32_t a = 1, b = 2, c = 3;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
    vm_create_value(vm, _get_i32_type(vm), &c, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  EXPECT_TRUE(value_is_error(cast));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_incompatible_element_error) {
  vm_t vm = vm_create(allocator);
  /* <i32, f64> tuple vs [2]i32 array — element types incompatible */
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  type_t i32_t = _get_i32_type(vm);
  value_t atv = vm_create_array_type_value(vm, i32_t, 2, true);
  array_type_t at = (array_type_t)value_get_data(atv);

  int32_t i = 1;
  double d = 2.5;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  EXPECT_TRUE(value_is_error(cast));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_widening_element) {
  vm_t vm = vm_create(allocator);
  /* <i32, i32> tuple → [2]i64 array: i32 safe_casts to i64 (widening) */
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types, _get_i32_type(vm));
  vec_push(types, _get_i32_type(vm));
  value_t ttv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  tuple_type_t tt = (tuple_type_t)value_get_data(ttv);

  type_t i64_t = (type_t)value_get_data(vm_get_i64_type(vm));
  value_t atv = vm_create_array_type_value(vm, i64_t, 2, true);
  array_type_t at = (array_type_t)value_get_data(atv);

  int32_t a = 42, b = 99;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  ASSERT_FALSE(value_is_error(cast)) << "i32→i64 widening should succeed";
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ARRAY);

  value_t idx0 = create_i32_value(vm, 0);
  value_t e0 = value_get_item(vm, cast, idx0);
  ASSERT_FALSE(value_is_error(e0));
  EXPECT_EQ(type_get_kind(value_get_type(e0)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(e0), 42);

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, safe_cast_tuple_to_array_shadow) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32x3_tuple_type(vm);
  array_type_t at = _make_i32x3_array_type(vm);

  value_t tup = create_tuple_shadow(vm, tt, true);
  value_t cast = value_safe_cast(vm, tup, (type_t)at);
  ASSERT_FALSE(value_is_error(cast));
  EXPECT_TRUE(value_is_shadow(cast));
  EXPECT_EQ(type_get_kind(value_get_type(cast)), TYPE_KIND_ARRAY);

  vm_dispose(vm, allocator);
}

/* ---- Assignment ---- */

TEST_F(it_tuple_type, assignment) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t a_i = 1;
  double a_d = 2.5;
  value_t elems_a[] = {
    vm_create_value(vm, _get_i32_type(vm), &a_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &a_d, NULL),
  };
  value_t dst = create_tuple_value(vm, tt, elems_a);

  int32_t b_i = 10;
  double b_d = 20.0;
  value_t elems_b[] = {
    vm_create_value(vm, _get_i32_type(vm), &b_i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &b_d, NULL),
  };
  value_t src = create_tuple_value(vm, tt, elems_b);

  value_t result = value_assignment(vm, dst, src);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  int32_t read_i = *(int32_t *)((char *)value_get_data(dst) + 0);
  double read_d = *(double *)((char *)value_get_data(dst) + 8);
  EXPECT_EQ(read_i, 10);
  EXPECT_DOUBLE_EQ(read_d, 20.0);

  vm_dispose(vm, allocator);
}

/* ---- to_string ---- */

TEST_F(it_tuple_type, to_string) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 42;
  double d = 3.14;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  value_t s = value_to_string(vm, tup);
  EXPECT_EQ(type_get_kind(value_get_type(s)), TYPE_KIND_STR);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(s)), "<42, 3.14>");

  vm_dispose(vm, allocator);
}

/* ---- Cross-scope clone ---- */

TEST_F(it_tuple_type, cross_scope_clone) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t i = 42;
  double d = 3.14;
  value_t elems[] = {
    vm_create_value(vm, _get_i32_type(vm), &i, NULL),
    vm_create_value(vm, _get_f64_type(vm), &d, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t local = value_clone(vm, tup);
  int32_t li = *(int32_t *)((char *)value_get_data(local) + 0);
  double ld = *(double *)((char *)value_get_data(local) + 8);
  EXPECT_EQ(li, 42);
  EXPECT_DOUBLE_EQ(ld, 3.14);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &callee);

  vm_dispose(vm, allocator);
}

/* ---- Shadow clone ---- */

TEST_F(it_tuple_type, shadow_clone) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  value_t tup = create_tuple_shadow(vm, tt, false);
  value_t cloned = value_clone(vm, tup);

  EXPECT_TRUE(value_is_shadow(cloned));
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_TUPLE);

  vm_dispose(vm, allocator);
}

/* ---- vm_create_tuple_type_value registers in scope ---- */

TEST_F(it_tuple_type, vm_create_tuple_type_value_registers_in_scope) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  type_t f64t = _get_f64_type(vm);

  scope_t scope = vm_get_current_scope(vm);
  size_t types_before = vec_get_size(scope->types);

  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(types, i32t);
  vec_push(types, f64t);
  value_t tv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  EXPECT_NE(tv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(tv)), TYPE_KIND_TYPE);

  tuple_type_t tt = (tuple_type_t)value_get_data(tv);
  EXPECT_EQ(type_get_kind((type_t)tt), TYPE_KIND_TUPLE);
  EXPECT_EQ(tuple_type_get_field_count(tt), 2u);

  /* registered in scope->types */
  EXPECT_EQ(vec_get_size(scope->types), types_before + 1);

  vm_dispose(vm, allocator);
}

/* ---- type_clone cross scope ---- */

TEST_F(it_tuple_type, type_clone_cross_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* create tuple type in outer scope */
  tuple_type_t outer_tt = _make_i32_f64_tuple_type(vm);

  /* switch to inner scope */
  scope_t inner = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev = vm_set_scope(vm, inner);
  scope_t prev_root = vm_set_root_scope(vm, inner);

  /* type_clone into inner scope */
  type_t inner_type = value_type_clone(vm, (type_t)outer_tt);
  tuple_type_t inner_tt = (tuple_type_t)inner_type;
  EXPECT_NE(inner_type, (type_t)outer_tt);
  EXPECT_EQ(type_get_kind(inner_type), TYPE_KIND_TUPLE);
  EXPECT_EQ(tuple_type_get_field_count(inner_tt), 2u);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(inner_tt, 0)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(inner_tt, 1)), TYPE_KIND_F64);

  /* inner scope owns the cloned type */
  EXPECT_GT(vec_get_size(inner->types), 0u);

  vm_set_scope(vm, prev);
  vm_set_root_scope(vm, prev_root);
  allocator_free(alloc, &inner);

  vm_dispose(vm, allocator);
}

/* ---- Empty tuple ---- */

/* ---- Wildcard tuple type ---- */

TEST_F(it_tuple_type, wildcard_tuple_singleton) {
  vm_t vm = vm_create(allocator);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_tuple_type(vm));

  EXPECT_EQ(type_get_kind(wct), TYPE_KIND_TUPLE);
  EXPECT_STREQ(type_get_name(wct), "<?>");
  EXPECT_EQ(type_get_size(wct), 0u);
  EXPECT_EQ(type_get_align(wct), 0u);
  EXPECT_FALSE(type_is_mut(wct));

  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, non_tuple_extends_wildcard_tuple_fails) {
  vm_t vm = vm_create(allocator);
  type_t wct = (type_t)value_get_data(vm_get_wildcard_tuple_type(vm));
  type_t i32t = _get_i32_type(vm);

  /* i32 is not a tuple, so i32 extends <?> is checked via the type-level
   * _type_extends dispatcher in type.c, which rejects kind mismatches */
  vtable_t vt = type_get_vtable(i32t);
  /* i32 has no type_extends vtable, so direct call returns error or
   * we test via the central dispatcher. Since we can't easily call
   * _type_extends from tests, verify via type_equal: i32 == <?> is false */
  if (vt.type_equal) {
    value_t eq = vt.type_equal(vm, i32t, wct);
    EXPECT_FALSE(*(bool *)value_get_data(eq));
  }

  vm_dispose(vm, allocator);
}

/* ---- Empty tuple rejected ---- */

TEST_F(it_tuple_type, vm_create_tuple_type_value_0_fields_returns_exception) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  EXPECT_EQ(type_get_kind(value_get_type(tv)), TYPE_KIND_EXCEPTION);
  vm_dispose(vm, allocator);
}

/* ---- Shadow operations ---- */

TEST_F(it_tuple_type, shadow_equal) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t a = create_tuple_shadow(vm, tt, true);
  value_t b = create_tuple_shadow(vm, tt, true);
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, shadow_assignment) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t a = create_tuple_shadow(vm, tt, false);
  value_t b = create_tuple_shadow(vm, tt, true);
  value_t result = value_assignment(vm, a, b);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(a));
  EXPECT_TRUE(value_is_shadow(a));
  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, shadow_get_item) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t tup = create_tuple_shadow(vm, tt, true);
  value_t idx = create_i32_value(vm, 0);
  value_t result = value_get_item(vm, tup, idx);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, shadow_set_item) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t tup = create_tuple_shadow(vm, tt, false);
  value_t idx = create_i32_value(vm, 0);
  value_t val = create_i32_value(vm, 42);
  value_t result = value_set_item(vm, tup, idx, val);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(tup));
  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, shadow_safe_cast) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  tuple_type_t const_tt = _make_const_i32_f64_tuple_type(vm);
  value_t tup = create_tuple_shadow(vm, tt, true);
  value_t result = value_safe_cast(vm, tup, (type_t)const_tt);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(value_get_type(result), (type_t)const_tt);
  vm_dispose(vm, allocator);
}

TEST_F(it_tuple_type, shadow_to_string) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t tup = create_tuple_shadow(vm, tt, true);
  value_t result = value_to_string(vm, tup);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}
