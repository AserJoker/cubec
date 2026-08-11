#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_str_type : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }
  type_t _get_const_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_const_str_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }

  string_t _str_handle(value_t v) {
    return *(string_t *)value_get_data(v);
  }

  const char *_read_str(value_t v) {
    return string_get(_str_handle(v));
  }
};

/* ---- create ---- */

TEST_F(it_str_type, create_value) {
  vm_t vm = vm_create(allocator);
  value_t v = create_str_value(vm, "hello");

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
  EXPECT_STREQ(_read_str(v), "hello");
  EXPECT_TRUE(value_is_own(v));
  EXPECT_TRUE(value_is_initialized(v));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, create_empty) {
  vm_t vm = vm_create(allocator);
  value_t v = create_str_value(vm, "");

  EXPECT_STREQ(_read_str(v), "");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- equal ---- */

TEST_F(it_str_type, equal_same) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "abc");
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, equal_different) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "xyz");
  value_t result = value_equal(vm, a, b);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, equal_integer_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_i32_value(vm, 42);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, equal_shadow) {
  vm_t vm = vm_create(allocator);
  type_t strt = _get_str_type(vm);
  value_t a = vm_create_value_shadow(vm, strt, NULL, true);
  value_t b = create_str_value(vm, "abc");
  value_t result = value_equal(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- add (concatenation) ---- */

TEST_F(it_str_type, add_concat) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  value_t b = create_str_value(vm, " world");
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_STR);
  EXPECT_STREQ(_read_str(result), "hello world");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, add_empty) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "");
  value_t result = value_add(vm, a, b);

  EXPECT_STREQ(_read_str(result), "abc");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, add_integer_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_i32_value(vm, 42);
  value_t result = value_add(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, add_shadow) {
  vm_t vm = vm_create(allocator);
  type_t strt = _get_str_type(vm);
  value_t a = vm_create_value_shadow(vm, strt, NULL, true);
  value_t b = create_str_value(vm, "abc");
  value_t result = value_add(vm, a, b);

  EXPECT_TRUE(value_is_shadow(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- relational ---- */

TEST_F(it_str_type, gt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "b");
  value_t b = create_str_value(vm, "a");
  value_t result = value_gt(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, lt) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t b = create_str_value(vm, "b");
  value_t result = value_lt(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, ne) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "xyz");
  value_t result = value_ne(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, ge) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "abc");
  value_t result = value_ge(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, le) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t b = create_str_value(vm, "abc");
  value_t result = value_le(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- lnot ---- */

TEST_F(it_str_type, lnot_empty) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "");
  value_t result = value_lnot(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, lnot_nonempty) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "abc");
  value_t result = value_lnot(vm, a);

  EXPECT_FALSE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- unsupported operators ---- */

TEST_F(it_str_type, sub_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t b = create_str_value(vm, "b");
  value_t result = value_sub(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, mul_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t b = create_str_value(vm, "b");
  value_t result = value_mul(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, band_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t b = create_str_value(vm, "b");
  value_t result = value_band(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, neg_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t result = value_neg(vm, a);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, shl_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "a");
  value_t b = create_str_value(vm, "b");
  value_t result = value_shl(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- safe_cast ---- */

TEST_F(it_str_type, safe_cast_to_str_identity) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  type_t strt = _get_str_type(vm);
  value_t result = value_safe_cast(vm, a, strt);

  EXPECT_EQ(result, a);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, safe_cast_to_const_str) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  type_t cstrt = _get_const_str_type(vm);
  value_t result = value_safe_cast(vm, a, cstrt);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_STR);
  EXPECT_FALSE(type_is_mut(value_get_type(result)));
  EXPECT_STREQ(_read_str(result), "hello");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, safe_cast_to_void_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  type_t vt = _get_void_type(vm);
  value_t result = value_safe_cast(vm, a, vt);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, const_str_safe_cast_to_str_error) {
  vm_t vm = vm_create(allocator);
  type_t cstrt = _get_const_str_type(vm);
  value_t a = vm_create_value_shadow(vm, cstrt, NULL, true);
  type_t strt = _get_str_type(vm);
  value_t result = value_safe_cast(vm, a, strt);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- assignment ---- */

TEST_F(it_str_type, assign) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "");
  value_t b = create_str_value(vm, "hello");
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_STREQ(_read_str(a), "hello");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, assign_replaces_old) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "old");
  value_t b = create_str_value(vm, "new");
  value_t result = value_assignment(vm, a, b);

  EXPECT_STREQ(_read_str(a), "new");
  EXPECT_STREQ(_read_str(b), "new");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, const_str_assign_error) {
  vm_t vm = vm_create(allocator);
  type_t cstrt = _get_const_str_type(vm);
  value_t a = vm_create_value_shadow(vm, cstrt, NULL, true);
  value_t b = create_str_value(vm, "hello");
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, assign_integer_error) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "");
  value_t b = create_i32_value(vm, 42);
  value_t result = value_assignment(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- type-level equal/extends ---- */

TEST_F(it_str_type, type_equal_str) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_str_type(vm);
  value_t b = vm_get_str_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, type_equal_i32_error) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_str_type(vm);
  value_t b = vm_get_i32_type(vm);
  value_t result = value_equal(vm, a, b);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_ERROR);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_str_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  value_t a = vm_get_str_type(vm);
  value_t b = vm_get_wildcard_type(vm);
  value_t result = value_extends(vm, a, b);

  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- move via allocator ---- */

TEST_F(it_str_type, move) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  allocator_t alloc = vm_get_allocator(vm);

  value_t m = (value_t)alloc_move(alloc, a);

  EXPECT_STREQ(_read_str(m), "hello");
  /* source is cleared */
  EXPECT_EQ(value_get_data(a), nullptr);
  EXPECT_FALSE(value_is_own(a));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
