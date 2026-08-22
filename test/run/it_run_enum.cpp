#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/enum_type.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/program.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "run/run.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_enum : public CubecTest {
protected:
  type_t _get_i32_type() {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_u8_type() {
    return (type_t)value_get_data(vm_get_u8_type(vm));
  }

  value_t _run_source(const char *source) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    allocator_free(alloc, &tokens);
    if (!node) return NULL;
    value_t v = run_program(vm, node, false);
    allocator_free(alloc, &node);
    return v;
  }

  /* Run source and look up a name in the current scope */
  value_t _run_and_lookup(const char *source, const char *name) {
    value_t v = _run_source(source);
    if (value_is_abnormal(v)) return v;
    scope_t scope = vm_get_current_scope(vm);
    name_t n = scope_lookup(scope, name);
    return n ? n->ref : NULL;
  }
};

/* ---- Basic enum with auto-increment (default i32) ---- */

TEST_F(it_run_enum, basic_auto_increment) {
  value_t val = _run_and_lookup("enum Color { Red, Green, Blue }", "Color");
  ASSERT_NE(val, nullptr);
  ASSERT_FALSE(value_is_abnormal(val));

  /* Color should be a TYPE_KIND_TYPE wrapping an enum type */
  ASSERT_EQ(type_get_kind(value_get_type(val)), TYPE_KIND_TYPE);
  type_t color_type = (type_t)value_get_data(val);
  ASSERT_EQ(type_get_kind(color_type), TYPE_KIND_ENUM);

  /* underlying should be i32 */
  enum_type_t et = (enum_type_t)color_type;
  ASSERT_EQ(type_get_kind(enum_type_get_underlying(et)), TYPE_KIND_I32);

  /* check items via namespace access */
  value_t red_val = value_get_prop(vm, val, "Red");
  ASSERT_FALSE(value_is_abnormal(red_val));
  ASSERT_EQ(value_get_type(red_val), color_type);
  /* Red should have value 0 */
  int32_t red_data;
  memcpy(&red_data, value_get_data(red_val), sizeof(int32_t));
  EXPECT_EQ(red_data, 0);

  value_t green_val = value_get_prop(vm, val, "Green");
  ASSERT_FALSE(value_is_abnormal(green_val));
  int32_t green_data;
  memcpy(&green_data, value_get_data(green_val), sizeof(int32_t));
  EXPECT_EQ(green_data, 1);

  value_t blue_val = value_get_prop(vm, val, "Blue");
  ASSERT_FALSE(value_is_abnormal(blue_val));
  int32_t blue_data;
  memcpy(&blue_data, value_get_data(blue_val), sizeof(int32_t));
  EXPECT_EQ(blue_data, 2);
}

/* ---- Enum with explicit underlying type ---- */

TEST_F(it_run_enum, explicit_underlying_u8) {
  value_t val = _run_and_lookup("enum Status { Ok: u8, Error: u8 }", "Status");
  ASSERT_NE(val, nullptr);
  ASSERT_FALSE(value_is_abnormal(val));

  type_t status_type = (type_t)value_get_data(val);
  ASSERT_EQ(type_get_kind(status_type), TYPE_KIND_ENUM);
  enum_type_t et = (enum_type_t)status_type;
  ASSERT_EQ(type_get_kind(enum_type_get_underlying(et)), TYPE_KIND_U8);
}

/* ---- Enum with explicit values ---- */

TEST_F(it_run_enum, explicit_values) {
  value_t val = _run_and_lookup(
      "enum Flags { A: i32 = 10, B: i32 = 20, C: i32 = 30 }", "Flags");
  ASSERT_NE(val, nullptr);
  ASSERT_FALSE(value_is_abnormal(val));

  type_t flags_type = (type_t)value_get_data(val);

  value_t a_val = value_get_prop(vm, val, "A");
  ASSERT_FALSE(value_is_abnormal(a_val));
  int32_t a_data;
  memcpy(&a_data, value_get_data(a_val), sizeof(int32_t));
  EXPECT_EQ(a_data, 10);

  value_t b_val = value_get_prop(vm, val, "B");
  ASSERT_FALSE(value_is_abnormal(b_val));
  int32_t b_data;
  memcpy(&b_data, value_get_data(b_val), sizeof(int32_t));
  EXPECT_EQ(b_data, 20);
}

/* ---- Enum with mixed auto-increment after explicit value ---- */

TEST_F(it_run_enum, mixed_auto_increment) {
  value_t val = _run_and_lookup(
      "enum Color { Red = 5, Green, Blue }", "Color");
  ASSERT_NE(val, nullptr);
  ASSERT_FALSE(value_is_abnormal(val));

  type_t color_type = (type_t)value_get_data(val);

  value_t red = value_get_prop(vm, val, "Red");
  ASSERT_FALSE(value_is_abnormal(red));
  int32_t red_data;
  memcpy(&red_data, value_get_data(red), sizeof(int32_t));
  EXPECT_EQ(red_data, 5);

  value_t green = value_get_prop(vm, val, "Green");
  ASSERT_FALSE(value_is_abnormal(green));
  int32_t green_data;
  memcpy(&green_data, value_get_data(green), sizeof(int32_t));
  EXPECT_EQ(green_data, 6);

  value_t blue = value_get_prop(vm, val, "Blue");
  ASSERT_FALSE(value_is_abnormal(blue));
  int32_t blue_data;
  memcpy(&blue_data, value_get_data(blue), sizeof(int32_t));
  EXPECT_EQ(blue_data, 7);
}

/* ---- Duplicate item name should error ---- */

TEST_F(it_run_enum, duplicate_item_error) {
  value_t val = _run_source("enum Color { Red, Red }");
  ASSERT_TRUE(value_is_abnormal(val));
}

/* ---- Value type mismatch should error (safe_cast) ---- */

TEST_F(it_run_enum, value_type_mismatch_error) {
  value_t val = _run_source("enum Bad { X: i32 = \"hello\" }");
  ASSERT_TRUE(value_is_abnormal(val));
}

/* ---- Enum item access via namespace syntax ---- */

TEST_F(it_run_enum, namespace_access) {
  value_t val = _run_and_lookup("enum Direction { North, South, East, West }",
                                "Direction");
  ASSERT_NE(val, nullptr);
  ASSERT_FALSE(value_is_abnormal(val));

  /* Access Direction::North via run_expression */
  allocator_t alloc = vm_get_allocator(vm);
  vec_t tokens = resolve_token_list(vm, "test.cubec", "Direction::North");
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t expr = read_expression(vm, tokens, &position, "test.cubec");
  allocator_free(alloc, &tokens);
  ASSERT_NE(expr, nullptr);

  value_t north = run_expression(vm, expr, false);
  allocator_free(alloc, &expr);
  ASSERT_FALSE(value_is_abnormal(north));

  type_t dir_type = (type_t)value_get_data(val);
  ASSERT_EQ(value_get_type(north), dir_type);
  int32_t north_data;
  memcpy(&north_data, value_get_data(north), sizeof(int32_t));
  EXPECT_EQ(north_data, 0);
}
