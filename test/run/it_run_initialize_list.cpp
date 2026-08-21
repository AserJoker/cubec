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
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/struct_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ------------------------------------------------------------------ *
 *  it_run_initialize_list — end-to-end tests for initialize_list
 *  runner: typed struct (field reorder + safe_cast), typed tuple/array
 *  (positional + safe_cast), anonymous struct (named fields), anonymous
 *  tuple (positional), empty list → empty anonymous struct.
 * ------------------------------------------------------------------ */

class it_run_initialize_list : public CubecTest {
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

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(vm_get_allocator(vm), &tokens);
  }

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
  }

  /* Parse a source string into an expression node via lexer→parser */
  node_t _parse_expr(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_expression(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  /* Parse + run a source string as an expression */
  value_t _run_expr(const char *source, bool shadow = false) {
    node_t node = _parse_expr(source);
    value_t v = run_expression(vm, node, shadow);
    free_node(node);
    return v;
  }

  /* Build a Point struct type { x: i32, y: i32 } and bind as "Point" */
  value_t _make_point_type() {
    value_t tv = vm_create_struct_type_value(vm, "Point", true, "test");
    (void)vm_struct_add_field(vm, tv, "x", vm_get_i32_type(vm), true);
    (void)vm_struct_add_field(vm, tv, "y", vm_get_i32_type(vm), true);
    (void)vm_struct_seal(vm, tv);
    _bind("Point", tv);
    return tv;
  }

  /* Build a tuple type <i32, f64> and bind as "Pair" */
  value_t _make_pair_type() {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, (type_t)value_get_data(vm_get_i32_type(vm)));
    vec_push(types, (type_t)value_get_data(vm_get_f64_type(vm)));
    value_t tv = vm_create_tuple_type_value(vm, types, true);
    allocator_free(alloc, &types);
    _bind("Pair", tv);
    return tv;
  }

  /* Build an array type [3]i32 and bind as "Triple" */
  value_t _make_triple_type() {
    type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
    value_t tv = vm_create_array_type_value(vm, i32_t, create_i32_value(vm, 3), true);
    _bind("Triple", tv);
    return tv;
  }
};

/* ==================================================================
 *  Anonymous empty list .{} → empty anonymous struct
 * ================================================================== */

TEST_F(it_run_initialize_list, anon_empty_is_empty_struct) {
  value_t v = _run_expr(".{}");
  ASSERT_FALSE(value_is_abnormal(v)) << ".{} should produce a value";
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
  EXPECT_EQ(type_get_name(value_get_type(v)), nullptr);
  /* struct value has no fields — check via its type */
  struct_type_t st = (struct_type_t)value_get_type(v);
  EXPECT_EQ(vec_get_size(st->fields), 0u);
  EXPECT_TRUE(value_is_initialized(v));
}

/* ==================================================================
 *  Anonymous named fields → anonymous struct
 * ================================================================== */

TEST_F(it_run_initialize_list, anon_named_fields_struct) {
  value_t v = _run_expr(".{.x = 10, .y = 20}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
  EXPECT_EQ(type_get_name(value_get_type(v)), nullptr);
  struct_type_t st = (struct_type_t)value_get_type(v);
  EXPECT_EQ(vec_get_size(st->fields), 2u);

  /* x field */
  value_t fx = value_get_field(vm, v, "x");
  ASSERT_FALSE(value_is_abnormal(fx));
  EXPECT_EQ(*(int32_t *)value_get_data(fx), 10);

  /* y field */
  value_t fy = value_get_field(vm, v, "y");
  ASSERT_FALSE(value_is_abnormal(fy));
  EXPECT_EQ(*(int32_t *)value_get_data(fy), 20);
}

TEST_F(it_run_initialize_list, anon_named_fields_mixed_types) {
  value_t v = _run_expr(".{.a = 1, .b = 2.5}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);

  value_t fa = value_get_field(vm, v, "a");
  ASSERT_FALSE(value_is_abnormal(fa));
  EXPECT_EQ(type_get_kind(value_get_type(fa)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(fa), 1);

  value_t fb = value_get_field(vm, v, "b");
  ASSERT_FALSE(value_is_abnormal(fb));
  EXPECT_EQ(type_get_kind(value_get_type(fb)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(fb), 2.5);
}

/* ==================================================================
 *  Anonymous positional → anonymous tuple
 * ================================================================== */

TEST_F(it_run_initialize_list, anon_positional_tuple) {
  value_t v = _run_expr(".{1, 2, 3}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TUPLE);
  tuple_type_t tt = (tuple_type_t)value_get_type(v);
  EXPECT_EQ(tuple_type_get_field_count(tt), 3u);

  /* access each element via subscript */
  for (uint64_t i = 0; i < 3; i++) {
    value_t idx = create_u64_value(vm, i);
    value_t elem = value_get_item(vm, v, idx);
    ASSERT_FALSE(value_is_abnormal(elem));
    EXPECT_EQ(type_get_kind(value_get_type(elem)), TYPE_KIND_I32);
    EXPECT_EQ(*(int32_t *)value_get_data(elem), (int32_t)(i + 1));
  }
}

TEST_F(it_run_initialize_list, anon_positional_mixed_types_tuple) {
  value_t v = _run_expr(".{42, 3.14}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TUPLE);
  tuple_type_t tt = (tuple_type_t)value_get_type(v);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 0)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 1)), TYPE_KIND_F64);
}

/* ==================================================================
 *  Typed struct: field reorder + safe_cast
 * ================================================================== */

TEST_F(it_run_initialize_list, typed_struct_named_fields_in_order) {
  _make_point_type();
  value_t v = _run_expr(".Point{.x = 10, .y = 20}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name(value_get_type(v)), "Point");

  value_t fx = value_get_field(vm, v, "x");
  ASSERT_FALSE(value_is_abnormal(fx));
  EXPECT_EQ(*(int32_t *)value_get_data(fx), 10);

  value_t fy = value_get_field(vm, v, "y");
  ASSERT_FALSE(value_is_abnormal(fy));
  EXPECT_EQ(*(int32_t *)value_get_data(fy), 20);
}

TEST_F(it_run_initialize_list, typed_struct_named_fields_reordered) {
  /* Provide fields in reverse order; runner must reorder by declaration */
  _make_point_type();
  value_t v = _run_expr(".Point{.y = 99, .x = 1}");
  ASSERT_FALSE(value_is_abnormal(v));

  value_t fx = value_get_field(vm, v, "x");
  ASSERT_FALSE(value_is_abnormal(fx));
  EXPECT_EQ(*(int32_t *)value_get_data(fx), 1);

  value_t fy = value_get_field(vm, v, "y");
  ASSERT_FALSE(value_is_abnormal(fy));
  EXPECT_EQ(*(int32_t *)value_get_data(fy), 99);
}

TEST_F(it_run_initialize_list, typed_struct_wrong_field_count_error) {
  _make_point_type();
  value_t v = _run_expr(".Point{.x = 1}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_struct_unknown_field_error) {
  _make_point_type();
  value_t v = _run_expr(".Point{.x = 1, .z = 2}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_struct_positional_on_struct_error) {
  _make_point_type();
  value_t v = _run_expr(".Point{1, 2}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_struct_safe_cast_widening) {
  /* i32 value → i64 field: safe_cast allows widening */
  value_t tv = vm_create_struct_type_value(vm, "Big", true, "test");
  (void)vm_struct_add_field(vm, tv, "v", vm_get_i64_type(vm), true);
  (void)vm_struct_seal(vm, tv);
  _bind("Big", tv);

  value_t v = _run_expr(".Big{.v = 42}");
  ASSERT_FALSE(value_is_abnormal(v)) << "i32→i64 widening should succeed";
  value_t fv = value_get_field(vm, v, "v");
  ASSERT_FALSE(value_is_abnormal(fv));
  EXPECT_EQ(type_get_kind(value_get_type(fv)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(fv), 42);
}

/* ==================================================================
 *  Typed tuple: positional + safe_cast
 * ================================================================== */

TEST_F(it_run_initialize_list, typed_tuple_positional) {
  _make_pair_type();
  value_t v = _run_expr(".Pair{42, 3.14}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TUPLE);
  tuple_type_t tt = (tuple_type_t)value_get_type(v);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 0)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(tuple_type_get_element_type(tt, 1)), TYPE_KIND_F64);

  /* verify values */
  value_t idx0 = create_u64_value(vm, 0);
  value_t e0 = value_get_item(vm, v, idx0);
  ASSERT_FALSE(value_is_abnormal(e0));
  EXPECT_EQ(*(int32_t *)value_get_data(e0), 42);

  value_t idx1 = create_u64_value(vm, 1);
  value_t e1 = value_get_item(vm, v, idx1);
  ASSERT_FALSE(value_is_abnormal(e1));
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(e1), 3.14);
}

TEST_F(it_run_initialize_list, typed_tuple_wrong_count_error) {
  _make_pair_type();
  value_t v = _run_expr(".Pair{1}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_tuple_named_fields_error) {
  _make_pair_type();
  value_t v = _run_expr(".Pair{.a = 1, .b = 2}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_tuple_incompatible_type_error) {
  /* str → i32 safe_cast fails */
  _make_pair_type();
  value_t v = _run_expr(".Pair{\"hello\", 3.14}");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Typed array: positional + safe_cast
 * ================================================================== */

TEST_F(it_run_initialize_list, typed_array_positional) {
  _make_triple_type();
  value_t v = _run_expr(".Triple{10, 20, 30}");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_ARRAY);
  array_type_t at = (array_type_t)value_get_type(v);
  EXPECT_EQ(array_type_get_count_value(at), 3u);

  for (uint64_t i = 0; i < 3; i++) {
    value_t idx = create_u64_value(vm, i);
    value_t elem = value_get_item(vm, v, idx);
    ASSERT_FALSE(value_is_abnormal(elem));
    EXPECT_EQ(*(int32_t *)value_get_data(elem), (int32_t)((i + 1) * 10));
  }
}

TEST_F(it_run_initialize_list, typed_array_wrong_count_error) {
  _make_triple_type();
  value_t v = _run_expr(".Triple{1, 2}");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_initialize_list, typed_array_named_fields_error) {
  _make_triple_type();
  value_t v = _run_expr(".Triple{.a = 1, .b = 2, .c = 3}");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Shadow mode
 * ================================================================== */

TEST_F(it_run_initialize_list, anon_struct_shadow) {
  value_t v = _run_expr(".{.x = 1, .y = 2}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
}

TEST_F(it_run_initialize_list, anon_tuple_shadow) {
  value_t v = _run_expr(".{1, 2, 3}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TUPLE);
}

TEST_F(it_run_initialize_list, anon_empty_shadow) {
  value_t v = _run_expr(".{}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
}

TEST_F(it_run_initialize_list, typed_struct_shadow) {
  _make_point_type();
  value_t v = _run_expr(".Point{.x = 1, .y = 2}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STRUCT);
}

TEST_F(it_run_initialize_list, typed_tuple_shadow) {
  _make_pair_type();
  value_t v = _run_expr(".Pair{1, 2.5}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TUPLE);
}

TEST_F(it_run_initialize_list, typed_array_shadow) {
  _make_triple_type();
  value_t v = _run_expr(".Triple{1, 2, 3}", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_ARRAY);
}

/* ==================================================================
 *  Unsupported type kind for initialize_list
 * ================================================================== */

TEST_F(it_run_initialize_list, typed_i32_unsupported_error) {
  /* i32 is not a composite type — initialize_list should fail */
  _bind("MyInt", vm_get_i32_type(vm));
  value_t v = _run_expr(".MyInt{1}");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Undeclared type name resolves to error
 * ================================================================== */

TEST_F(it_run_initialize_list, typed_undeclared_type_error) {
  value_t v = _run_expr(".NoSuchType{.x = 1}");
  EXPECT_TRUE(value_is_abnormal(v));
}
