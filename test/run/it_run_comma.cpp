#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/str_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/error.h"
#include "engine/interrupt_type.h"
#include "engine/result_type.h"
#include "engine/callable_type.h"
#include "engine/union_type.h"
#include "cubec/expression_comma.h"
#include "cubec/literal_identifier.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/location.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_comma : public CubecTest {
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
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }

  /* Parse a source string into an expression node */
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
};

/* ================================================================== *
 *  Basic comma expression                                             *
 * ================================================================== */

TEST_F(it_run_comma, two_literals_returns_last) {
  /* 1, 2 → 2 */
  value_t v = _run_expr("1, 2");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 2);
}

TEST_F(it_run_comma, three_literals_returns_last) {
  /* 1, 2, 3 → 3 (parsed as comma(1, comma(2, 3))) */
  value_t v = _run_expr("1, 2, 3");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 3);
}

TEST_F(it_run_comma, mixed_types_returns_last) {
  /* 1, "hello" → "hello" */
  value_t v = _run_expr("1, \"hello\"");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
}

/* ================================================================== *
 *  Comma with identifiers (side effects)                              *
 * ================================================================== */

TEST_F(it_run_comma, with_identifiers_returns_last_value) {
  _bind("x", create_i32_value(vm, 10));
  _bind("y", create_i32_value(vm, 20));
  /* x, y → y's value = 20 */
  value_t v = _run_expr("x, y");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 20);
}

TEST_F(it_run_comma, left_side_assignment_executed) {
  _bind("x", create_i32_value(vm, 0));
  _bind("y", create_i32_value(vm, 42));
  /* x = y, y — left side assigns y to x, then returns y */
  value_t v = _run_expr("x = y, y");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  /* Verify side effect: x should now be 42 */
  value_t x_val = _run_expr("x");
  ASSERT_NE(x_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(x_val), 42);
}

/* ================================================================== *
 *  Comma with expressions                                             *
 * ================================================================== */

TEST_F(it_run_comma, with_binary_expressions) {
  _bind("a", create_i32_value(vm, 5));
  _bind("b", create_i32_value(vm, 3));
  /* a + b, a * b → a*b = 15 */
  value_t v = _run_expr("a + b, a * b");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 15);
}

/* ================================================================== *
 *  Comma with interrupt propagation                                   *
 * ================================================================== */

TEST_F(it_run_comma, left_interrupt_propagates) {
  /* If left side produces interrupt, the whole comma expression
   * should propagate it without evaluating right side */
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  value_t rv = vm_create_result_type_value(vm, i32_type, error_type);
  value_t err_result = vm_create_union_value(vm, rv, "_error",
                                              create_error_value(vm, 1, "fail"));
  _bind("r", err_result);

  type_t result_type = (type_t)value_get_data(rv);
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t pvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  callable_type_t ct = callable_type_create(alloc, param_types, result_type,
                                             false, true, "<module>");
  allocator_free(alloc, &param_types);
  vec_push(vm_get_types(vm), ct);
  value_t func_val = create_callable_value(vm, ct,
      (value_t (*)(vm_t, value_t, size_t, value_t *))NULL, "test_fn");

  value_t prev_func = vm_set_current_func(vm, func_val);
  /* r.?, 42 — r.? produces interrupt, should propagate */
  value_t v = _run_expr("r.?, 42");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
}

TEST_F(it_run_comma, right_interrupt_propagates) {
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  value_t rv = vm_create_result_type_value(vm, i32_type, error_type);
  value_t err_result = vm_create_union_value(vm, rv, "_error",
                                              create_error_value(vm, 1, "fail"));
  _bind("r", err_result);

  type_t result_type = (type_t)value_get_data(rv);
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t pvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  callable_type_t ct = callable_type_create(alloc, param_types, result_type,
                                             false, true, "<module>");
  allocator_free(alloc, &param_types);
  vec_push(vm_get_types(vm), ct);
  value_t func_val = create_callable_value(vm, ct,
      (value_t (*)(vm_t, value_t, size_t, value_t *))NULL, "test_fn");

  value_t prev_func = vm_set_current_func(vm, func_val);
  /* 42, r.? — left is ok, right produces interrupt */
  value_t v = _run_expr("42, r.?");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
}

/* ================================================================== *
 *  Comma with exception propagation                                   *
 * ================================================================== */

TEST_F(it_run_comma, left_exception_propagates) {
  /* If left side produces exception, propagate without evaluating right */
  _bind("x", create_i32_value(vm, 42));
  /* x.! on i32 produces exception */
  value_t v = _run_expr("x.!, 42");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_comma, right_exception_propagates) {
  _bind("x", create_i32_value(vm, 42));
  /* 42, x.! — left is ok, right produces exception */
  value_t v = _run_expr("42, x.!");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  Shadow mode                                                        *
 * ================================================================== */

TEST_F(it_run_comma, shadow_returns_last_shadow) {
  _bind("x", create_i32_value(vm, 10));
  _bind("y", create_i32_value(vm, 20));
  /* Shadow mode: evaluate both, return last */
  node_t node = _parse_expr("x, y");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_comma, shadow_with_assignment) {
  _bind("x", create_i32_value(vm, 0));
  _bind("y", create_i32_value(vm, 42));
  /* Shadow: x = y, y — assignment returns void shadow, then y returns i32 shadow */
  node_t node = _parse_expr("x = y, y");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

/* ================================================================== *
 *  Comma with void left operand                                       *
 * ================================================================== */

TEST_F(it_run_comma, assignment_left_returns_right) {
  /* Assignment returns void, so comma(assignment, expr) returns expr */
  _bind("x", create_i32_value(vm, 0));
  value_t v = _run_expr("x = 10, 42");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  /* Side effect: x should be 10 */
  value_t x_val = _run_expr("x");
  ASSERT_NE(x_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(x_val), 10);
}
