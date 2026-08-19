#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/exception_type.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/literal_nil.h"
#include "cubec/literal_undefined.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/location.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_literal : public CubecTest {
protected:
  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
  }

  void free_node(node_t &node) {
    if (node) allocator_free(vm_get_allocator(vm), &node);
  }
};

/* ---- run_literal_numeric ---- */

TEST_F(it_run_literal, numeric_i32_default) {
  node_t node = create_literal_numeric(vm, _loc(), "42",
      CUBEC_LITERAL_NUMERIC_KIND_INTEGER, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  value_t v = run_literal_numeric(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
  free_node(node);
}

TEST_F(it_run_literal, numeric_i64) {
  node_t node = create_literal_numeric(vm, _loc(), "9999999999",
      CUBEC_LITERAL_NUMERIC_KIND_INTEGER, CUBEC_LITERAL_NUMERIC_TYPE_I64);
  value_t v = run_literal_numeric(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(v), 9999999999LL);
  free_node(node);
}

TEST_F(it_run_literal, numeric_u8) {
  node_t node = create_literal_numeric(vm, _loc(), "200",
      CUBEC_LITERAL_NUMERIC_KIND_INTEGER, CUBEC_LITERAL_NUMERIC_TYPE_U8);
  value_t v = run_literal_numeric(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U8);
  EXPECT_EQ(*(uint8_t *)value_get_data(v), 200);
  free_node(node);
}

TEST_F(it_run_literal, numeric_f64_default) {
  node_t node = create_literal_numeric(vm, _loc(), "3.14",
      CUBEC_LITERAL_NUMERIC_KIND_FLOAT, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  value_t v = run_literal_numeric(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 3.14);
  free_node(node);
}

TEST_F(it_run_literal, numeric_f32) {
  node_t node = create_literal_numeric(vm, _loc(), "2.5",
      CUBEC_LITERAL_NUMERIC_KIND_FLOAT, CUBEC_LITERAL_NUMERIC_TYPE_F32);
  value_t v = run_literal_numeric(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F32);
  EXPECT_FLOAT_EQ(*(float *)value_get_data(v), 2.5f);
  free_node(node);
}

TEST_F(it_run_literal, numeric_shadow) {
  node_t node = create_literal_numeric(vm, _loc(), "42",
      CUBEC_LITERAL_NUMERIC_KIND_INTEGER, CUBEC_LITERAL_NUMERIC_TYPE_I32);
  value_t v = run_literal_numeric(vm, node, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(node);
}

/* ---- run_literal_string ---- */

TEST_F(it_run_literal, string_basic) {
  node_t node = create_literal_string(vm, _loc(), "hello");
  value_t v = run_literal_string(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
  string_t *sp = (string_t *)value_get_data(v);
  EXPECT_STREQ(string_get(*sp), "hello");
  free_node(node);
}

TEST_F(it_run_literal, string_shadow) {
  node_t node = create_literal_string(vm, _loc(), "hello");
  value_t v = run_literal_string(vm, node, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(node);
}

/* ---- run_literal_char ---- */

TEST_F(it_run_literal, char_basic) {
  node_t node = create_literal_char(vm, _loc(), 'A');
  value_t v = run_literal_char(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U8);
  EXPECT_EQ(*(uint8_t *)value_get_data(v), 65);
  free_node(node);
}

TEST_F(it_run_literal, char_shadow) {
  node_t node = create_literal_char(vm, _loc(), 'Z');
  value_t v = run_literal_char(vm, node, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U8);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(node);
}

/* ---- run_literal_nil ---- */

TEST_F(it_run_literal, nil_basic) {
  node_t node = create_literal_nil(vm, _loc());
  value_t v = run_literal_nil(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_NIL);
  void **data = (void **)value_get_data(v);
  EXPECT_EQ(*data, nullptr);
  free_node(node);
}

TEST_F(it_run_literal, nil_shadow) {
  node_t node = create_literal_nil(vm, _loc());
  value_t v = run_literal_nil(vm, node, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_NIL);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(node);
}

/* ---- run_literal_undefined ---- */

TEST_F(it_run_literal, undefined_returns_exception) {
  node_t node = create_literal_undefined(vm, _loc());
  value_t v = run_literal_undefined(vm, node, false);

  EXPECT_TRUE(value_is_abnormal(v));
  free_node(node);
}

TEST_F(it_run_literal, undefined_shadow_returns_exception) {
  node_t node = create_literal_undefined(vm, _loc());
  value_t v = run_literal_undefined(vm, node, true);

  EXPECT_TRUE(value_is_abnormal(v));
  free_node(node);
}

/* ---- run_literal_identifier ---- */

TEST_F(it_run_literal, identifier_found) {
  scope_t scope = vm_get_current_scope(vm);

  value_t i32_val = create_i32_value(vm, 100);
  name_t name = name_create(scope->allocator, i32_val);
  strmap_insert(scope->names, "x", name);

  node_t node = create_literal_identifier(vm, _loc(), "x");
  value_t v = run_literal_identifier(vm, node, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 100);
  free_node(node);
}

TEST_F(it_run_literal, identifier_not_found) {
  node_t node = create_literal_identifier(vm, _loc(), "nonexistent");
  value_t v = run_literal_identifier(vm, node, false);

  EXPECT_TRUE(value_is_abnormal(v));
  free_node(node);
}

TEST_F(it_run_literal, identifier_shadow) {
  scope_t scope = vm_get_current_scope(vm);

  value_t i32_val = create_i32_value(vm, 42);
  name_t name = name_create(scope->allocator, i32_val);
  strmap_insert(scope->names, "y", name);

  node_t node = create_literal_identifier(vm, _loc(), "y");
  value_t v = run_literal_identifier(vm, node, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(node);
}
