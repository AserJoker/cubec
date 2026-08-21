#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/scope.h"
#include "core/string.h"
#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_value_spread : public CubecTest {
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

  /* Helper: create a (i32, f64) tuple type + value */
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

  /* Helper: create a [3]i32 array type */
  array_type_t _make_i32_array3_type(vm_t vm) {
    value_t tv = vm_create_array_type_value(vm, _get_i32_type(vm), create_i32_value(vm, 3), true);
    return (array_type_t)value_get_data(tv);
  }
};

/* ---- Tuple spread ---- */

TEST_F(it_value_spread, tuple_spread_returns_elements) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);

  int32_t a = 42;
  double b = 3.14;
  value_t elems[] = {
      vm_create_value(vm, _get_i32_type(vm), &a, NULL),
      vm_create_value(vm, _get_f64_type(vm), &b, NULL),
  };
  value_t tup = create_tuple_value(vm, tt, elems);

  vec_t spread = value_spread(vm, tup);
  ASSERT_NE(spread, nullptr);
  EXPECT_EQ(vec_get_size(spread), 2u);

  /* first element should be i32 with value 42 */
  value_t e0 = (value_t)vec_get(spread, 0);
  EXPECT_EQ(type_get_kind(value_get_type(e0)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(e0), 42);

  /* second element should be f64 with value 3.14 */
  value_t e1 = (value_t)vec_get(spread, 1);
  EXPECT_EQ(type_get_kind(value_get_type(e1)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(e1), 3.14);

  allocator_free(vm_get_allocator(vm), &spread);
  vm_dispose(vm, allocator);
}

TEST_F(it_value_spread, tuple_spread_empty_tuple) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* create empty tuple type (unit) */
  vec_init_t vi = {.auto_dispose = false};
  vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_tuple_type_value(vm, types, true);
  allocator_free(alloc, &types);
  tuple_type_t tt = (tuple_type_t)value_get_data(tv);

  /* create empty tuple value (size=0, data=NULL) */
  value_t tup = create_tuple_value(vm, tt, NULL);

  vec_t spread = value_spread(vm, tup);
  ASSERT_NE(spread, nullptr);
  EXPECT_EQ(vec_get_size(spread), 0u);

  allocator_free(alloc, &spread);
  vm_dispose(vm, allocator);
}

TEST_F(it_value_spread, tuple_spread_shadow_returns_shadow_elements) {
  vm_t vm = vm_create(allocator);
  tuple_type_t tt = _make_i32_f64_tuple_type(vm);
  value_t shadow = create_tuple_shadow(vm, tt, true);

  vec_t spread = value_spread(vm, shadow);
  ASSERT_NE(spread, nullptr);
  EXPECT_EQ(vec_get_size(spread), 2u);

  /* shadow spread produces shadow elements */
  value_t e0 = (value_t)vec_get(spread, 0);
  EXPECT_TRUE(value_is_shadow(e0));
  EXPECT_EQ(type_get_kind(value_get_type(e0)), TYPE_KIND_I32);

  value_t e1 = (value_t)vec_get(spread, 1);
  EXPECT_TRUE(value_is_shadow(e1));
  EXPECT_EQ(type_get_kind(value_get_type(e1)), TYPE_KIND_F64);

  allocator_free(vm_get_allocator(vm), &spread);
  vm_dispose(vm, allocator);
}

/* ---- Array spread ---- */

TEST_F(it_value_spread, array_spread_returns_elements) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array3_type(vm);

  int32_t v0 = 10, v1 = 20, v2 = 30;
  value_t elems[] = {
      vm_create_value(vm, _get_i32_type(vm), &v0, NULL),
      vm_create_value(vm, _get_i32_type(vm), &v1, NULL),
      vm_create_value(vm, _get_i32_type(vm), &v2, NULL),
  };
  value_t arr = create_array_value(vm, at, elems);

  vec_t spread = value_spread(vm, arr);
  ASSERT_NE(spread, nullptr);
  EXPECT_EQ(vec_get_size(spread), 3u);

  EXPECT_EQ(*(int32_t *)value_get_data((value_t)vec_get(spread, 0)), 10);
  EXPECT_EQ(*(int32_t *)value_get_data((value_t)vec_get(spread, 1)), 20);
  EXPECT_EQ(*(int32_t *)value_get_data((value_t)vec_get(spread, 2)), 30);

  allocator_free(vm_get_allocator(vm), &spread);
  vm_dispose(vm, allocator);
}

TEST_F(it_value_spread, array_spread_shadow_returns_null) {
  vm_t vm = vm_create(allocator);
  array_type_t at = _make_i32_array3_type(vm);
  value_t shadow = create_array_shadow(vm, at, true);

  vec_t spread = value_spread(vm, shadow);
  EXPECT_EQ(spread, nullptr);

  vm_dispose(vm, allocator);
}

/* ---- Non-spreadable type ---- */

TEST_F(it_value_spread, non_spreadable_type_returns_null) {
  vm_t vm = vm_create(allocator);
  int32_t val = 42;
  value_t i32_val = vm_create_value(vm, _get_i32_type(vm), &val, NULL);

  vec_t spread = value_spread(vm, i32_val);
  EXPECT_EQ(spread, nullptr);

  vm_dispose(vm, allocator);
}
