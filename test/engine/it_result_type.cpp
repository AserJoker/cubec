#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/exception_type.h"
#include "engine/error.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/callable_type.h"
#include "engine/pointer_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_result_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }
  type_t _get_error_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_error_type(vm));
  }

  /** Create result[i32, error] type */
  value_t _make_i32_result(vm_t vm) {
    return vm_create_result_type_value(vm, _get_i32_type(vm), _get_error_type(vm));
  }

  /** Create result[str, error] type */
  value_t _make_str_result(vm_t vm) {
    return vm_create_result_type_value(vm, _get_str_type(vm), _get_error_type(vm));
  }
};

/* ---- Type creation ---- */

TEST_F(it_result_type, create_result_type) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);

  ASSERT_NE(rv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(rv)), TYPE_KIND_TYPE);

  /* data points to the union_type_t */
  union_type_t ut = (union_type_t)value_get_data(rv);
  EXPECT_EQ(type_get_kind((type_t)ut), TYPE_KIND_UNION);
  EXPECT_STREQ(type_get_name((type_t)ut), "result[i32,error]");

  /* fields: _value:i32, _error:error */
  EXPECT_EQ(vec_get_size(union_type_get_fields(ut)), 2u);
  field_info_t fv = union_type_find_field(ut, "_value");
  field_info_t fe = union_type_find_field(ut, "_error");
  ASSERT_NE(fv, nullptr);
  ASSERT_NE(fe, nullptr);
  EXPECT_EQ(type_get_kind(field_info_get_type(fv)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(field_info_get_type(fe)), TYPE_KIND_STRUCT);
  EXPECT_TRUE(union_type_is_sealed(ut));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, result_fields_are_private) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  EXPECT_FALSE(union_type_is_field_pub(ut, "_value"));
  EXPECT_FALSE(union_type_is_field_pub(ut, "_error"));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Methods registration ---- */

TEST_F(it_result_type, ok_method_registered) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t ok_fn = (value_t)strmap_find(union_type_get_methods(ut), "ok");
  ASSERT_NE(ok_fn, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(ok_fn)), TYPE_KIND_CALLABLE);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, of_value_and_of_error_are_props) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  /* of_value and of_error are props (not methods) */
  value_t of_val = (value_t)strmap_find(union_type_get_props(ut), "of_value");
  value_t of_err = (value_t)strmap_find(union_type_get_props(ut), "of_error");
  ASSERT_NE(of_val, nullptr);
  ASSERT_NE(of_err, nullptr);
  /* they should NOT be in methods */
  EXPECT_EQ(strmap_find(union_type_get_methods(ut), "of_value"), nullptr);
  EXPECT_EQ(strmap_find(union_type_get_methods(ut), "of_error"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- of_value / of_error ---- */

TEST_F(it_result_type, of_value_creates_ok_result) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  /* get of_value callable via type_get_prop */
  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);
  value_t of_val_fn = value_get_prop(vm, type_val, "of_value");
  ASSERT_EQ(type_get_kind(value_get_type(of_val_fn)), TYPE_KIND_CALLABLE);

  /* call of_value(42) */
  value_t arg = create_i32_value(vm, 42);
  value_t argv[] = {arg};
  value_t result = value_call(vm, of_val_fn, 1, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_UNION);

  /* tag should be 0 (_value) */
  uint32_t tag = *(uint32_t *)value_get_data(result);
  EXPECT_EQ(tag, 0u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, of_error_creates_error_result) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t type_val = create_type_value(vm, (type_t)ut, NULL, false);
  value_t of_err_fn = value_get_prop(vm, type_val, "of_error");
  ASSERT_EQ(type_get_kind(value_get_type(of_err_fn)), TYPE_KIND_CALLABLE);

  /* call of_error(error value) */
  value_t err_val = create_error_value(vm, 1, "test error");
  value_t argv[] = {err_val};
  value_t result = value_call(vm, of_err_fn, 1, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_UNION);

  /* tag should be 1 (_error) */
  uint32_t tag = *(uint32_t *)value_get_data(result);
  EXPECT_EQ(tag, 1u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- ok() method ---- */

TEST_F(it_result_type, ok_returns_true_for_value_variant) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  /* create a result with _value active (tag=0) */
  value_t i32_val = create_i32_value(vm, 42);
  value_t result_val = create_union_value(vm, ut, 0, i32_val);

  /* call result.ok() via member_call */
  value_t ok_result = value_member_call(vm, result_val, "ok", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(ok_result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(ok_result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, ok_returns_false_for_error_variant) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  /* create a result with _error active (tag=1) */
  value_t err_val = create_error_value(vm, 1, "test error");
  value_t result_val = create_union_value(vm, ut, 1, err_val);

  value_t ok_result = value_member_call(vm, result_val, "ok", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(ok_result)), TYPE_KIND_BOOL);
  EXPECT_FALSE(*(bool *)value_get_data(ok_result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- value() method ---- */

TEST_F(it_result_type, value_returns_inner_when_ok) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t i32_val = create_i32_value(vm, 42);
  value_t result_val = create_union_value(vm, ut, 0, i32_val);

  value_t inner = value_member_call(vm, result_val, "value", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 42);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, value_panics_when_error) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t err_val = create_error_value(vm, 1, "test error");
  value_t result_val = create_union_value(vm, ut, 1, err_val);

  value_t inner = value_member_call(vm, result_val, "value", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- error() method ---- */

TEST_F(it_result_type, error_returns_inner_when_error) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t err_val = create_error_value(vm, 1, "test error");
  value_t result_val = create_union_value(vm, ut, 1, err_val);

  value_t inner = value_member_call(vm, result_val, "error", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_STRUCT); /* error struct */

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_result_type, error_panics_when_ok) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_i32_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  value_t i32_val = create_i32_value(vm, 42);
  value_t result_val = create_union_value(vm, ut, 0, i32_val);

  value_t inner = value_member_call(vm, result_val, "error", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- str result variant ---- */

TEST_F(it_result_type, str_result_of_value_and_ok) {
  vm_t vm = vm_create(allocator);
  value_t rv = _make_str_result(vm);
  union_type_t ut = (union_type_t)value_get_data(rv);

  EXPECT_STREQ(type_get_name((type_t)ut), "result[str,error]");

  value_t s = create_str_value(vm, "hello");
  value_t result_val = create_union_value(vm, ut, 0, s);

  value_t ok_result = value_member_call(vm, result_val, "ok", 0, NULL);
  EXPECT_TRUE(*(bool *)value_get_data(ok_result));

  value_t inner = value_member_call(vm, result_val, "value", 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
