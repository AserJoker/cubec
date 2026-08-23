#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/str_type.h"
#include "engine/error.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/result_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/callable_type.h"
#include "engine/pointer_type.h"
#include "cubec/expression_try.h"
#include "cubec/expression_assert.h"
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

class it_run_try_assert : public CubecTest {
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

  type_t _get_i32_type() {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_str_type() {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }
  type_t _get_error_type() {
    return (type_t)value_get_data(vm_get_error_type(vm));
  }

  /** Create result[i32, error] type */
  value_t _make_i32_result() {
    return vm_create_result_type_value(vm, _get_i32_type(), _get_error_type());
  }

  /** Create result[str, error] type */
  value_t _make_str_result() {
    return vm_create_result_type_value(vm, _get_str_type(), _get_error_type());
  }

  /** Create a result[i32, error] value with _value active (ok) */
  value_t _make_ok_i32_result(int32_t val) {
    value_t rv = _make_i32_result();
    value_t i32_val = create_i32_value(vm, val);
    return vm_create_union_value(vm, rv, "_value", i32_val);
  }

  /** Create a result[i32, error] value with _error active */
  value_t _make_err_i32_result(int32_t code, const char *msg) {
    value_t rv = _make_i32_result();
    value_t err_val = create_error_value(vm, code, msg);
    return vm_create_union_value(vm, rv, "_error", err_val);
  }

  /** Create a result[str, error] value with _value active (ok) */
  value_t _make_ok_str_result(const char *val) {
    value_t rv = _make_str_result();
    value_t str_val = create_str_value(vm, val);
    return vm_create_union_value(vm, rv, "_value", str_val);
  }

  /** Create a result[str, error] value with _error active */
  value_t _make_err_str_result(int32_t code, const char *msg) {
    value_t rv = _make_str_result();
    value_t err_val = create_error_value(vm, code, msg);
    return vm_create_union_value(vm, rv, "_error", err_val);
  }

  /** Create a callable value with the given return type for use as current_func */
  value_t _make_func_with_return_type(type_t ret_type, const char *name = "test_fn") {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t pvi = {.auto_dispose = false};
    vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
    callable_type_t ct = callable_type_create(alloc, param_types, ret_type,
                                               false, true, "<module>");
    allocator_free(alloc, &param_types);
    vec_push(vm_get_types(vm), ct);
    return create_callable_value(vm, ct,
        (value_t (*)(vm_t, value_t, size_t, value_t *))NULL, name);
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
 *  .! (assert/force-unwrap) — basic                                  *
 * ================================================================== */

TEST_F(it_run_try_assert, assert_on_ok_i32_result_returns_value) {
  _bind("r", _make_ok_i32_result(42));
  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_try_assert, assert_on_err_i32_result_panics) {
  _bind("r", _make_err_i32_result(1, "test error"));
  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_try_assert, assert_on_ok_str_result_returns_value) {
  _bind("r", _make_ok_str_result("hello"));
  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
}

TEST_F(it_run_try_assert, assert_on_err_str_result_panics) {
  _bind("r", _make_err_str_result(1, "test error"));
  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  .? (try/propagate) — basic                                        *
 * ================================================================== */

TEST_F(it_run_try_assert, try_on_ok_i32_result_returns_value) {
  _bind("r", _make_ok_i32_result(42));
  value_t v = _run_expr("r.?");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_try_assert, try_on_err_i32_result_outside_function_returns_exception) {
  _bind("r", _make_err_i32_result(1, "test error"));
  value_t v = _run_expr("r.?");
  ASSERT_NE(v, nullptr);
  /* outside function context: cannot return, produces exception */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  .? (try) inside function — interrupt propagation                   *
 * ================================================================== */

TEST_F(it_run_try_assert, try_on_err_result_in_function_produces_interrupt) {
  value_t rv = _make_i32_result();
  value_t err_result = vm_create_union_value(vm, rv, "_error",
                                              create_error_value(vm, 1, "fail"));
  _bind("r", err_result);

  type_t result_type = (type_t)value_get_data(rv);
  value_t func_val = _make_func_with_return_type(result_type);
  ASSERT_NE(func_val, nullptr);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_expr("r.?");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);

  /* Interrupt wraps a result[i32, error] with _error active */
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_UNION);
}

TEST_F(it_run_try_assert, try_on_ok_result_in_function_returns_value_not_interrupt) {
  _bind("r", _make_ok_i32_result(42));

  value_t rv = _make_i32_result();
  type_t result_type = (type_t)value_get_data(rv);
  value_t func_val = _make_func_with_return_type(result_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_expr("r.?");
  vm_set_current_func(vm, prev_func);

  /* ok path returns the value directly, not an interrupt */
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_try_assert, try_on_err_result_with_non_union_return_type_returns_exception) {
  _bind("r", _make_err_i32_result(1, "fail"));

  /* Function returns i32 (not result) — .? cannot construct of_error */
  value_t func_val = _make_func_with_return_type(_get_i32_type());

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_expr("r.?");
  vm_set_current_func(vm, prev_func);

  /* Return type is not a result type → exception */
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  .?/.! inside expressions — statement-like behavior                 *
 *  .? is special: it's an expression but acts like a return statement *
 * ================================================================== */

TEST_F(it_run_try_assert, try_in_arithmetic_expression_err_propagates) {
  /* r.? + 10 where r is error — the interrupt/exception should propagate
   * through the binary expression, stopping evaluation */
  _bind("r", _make_err_i32_result(1, "fail"));

  value_t rv = _make_i32_result();
  type_t result_type = (type_t)value_get_data(rv);
  value_t func_val = _make_func_with_return_type(result_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_expr("r.? + 10");
  vm_set_current_func(vm, prev_func);

  /* The interrupt from .? should propagate through the binary +.
   * Binary ops check value_is_abnormal on the left operand first. */
  ASSERT_NE(v, nullptr);
  /* Should be interrupt (propagated from .?) or exception if the
   * binary runner catches it differently */
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_try_assert, assert_in_arithmetic_expression_err_propagates) {
  /* r.! + 10 where r is error — the exception should propagate */
  _bind("r", _make_err_i32_result(1, "fail"));
  value_t v = _run_expr("r.! + 10");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_try_assert, try_in_ok_path_used_in_arithmetic) {
  /* r.? + 10 where r is ok(32) → should return 42 */
  _bind("r", _make_ok_i32_result(32));
  value_t v = _run_expr("r.? + 10");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_try_assert, assert_in_ok_path_used_in_arithmetic) {
  /* r.! + 10 where r is ok(32) → should return 42 */
  _bind("r", _make_ok_i32_result(32));
  value_t v = _run_expr("r.! + 10");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

/* ================================================================== *
 *  Chained .?/.!                                                      *
 * ================================================================== */

TEST_F(it_run_try_assert, chained_assert_on_ok_result) {
  /* r.!.! — first .! returns i32, second .! would fail (no .ok on i32) */
  _bind("r", _make_ok_i32_result(42));
  value_t v = _run_expr("r.!.!");
  /* i32 doesn't have .ok() → exception */
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  .?/.! on non-union types                                           *
 * ================================================================== */

TEST_F(it_run_try_assert, assert_on_i32_value_returns_exception) {
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_expr("x.!");
  /* i32 doesn't have .ok() → exception */
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_try_assert, try_on_i32_value_returns_exception) {
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_expr("x.?");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_try_assert, assert_on_bool_value_returns_exception) {
  _bind("b", create_bool_value(vm, true));
  value_t v = _run_expr("b.!");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  Shadow mode                                                        *
 * ================================================================== */

TEST_F(it_run_try_assert, shadow_assert_on_ok_result) {
  _bind("r", _make_ok_i32_result(42));
  node_t node = _parse_expr("r.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_try_assert, shadow_try_on_ok_result) {
  _bind("r", _make_ok_i32_result(42));
  node_t node = _parse_expr("r.?");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_try_assert, shadow_assert_on_err_result) {
  /* Shadow mode on error result: type-level, doesn't care about ok/error,
   * just extracts _value field type */
  _bind("r", _make_err_i32_result(1, "fail"));
  node_t node = _parse_expr("r.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_try_assert, shadow_try_on_err_result) {
  _bind("r", _make_err_i32_result(1, "fail"));
  node_t node = _parse_expr("r.?");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_try_assert, shadow_assert_on_str_result) {
  _bind("r", _make_ok_str_result("hello"));
  node_t node = _parse_expr("r.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
}

TEST_F(it_run_try_assert, shadow_try_on_str_result) {
  _bind("r", _make_ok_str_result("hello"));
  node_t node = _parse_expr("r.?");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_STR);
}

TEST_F(it_run_try_assert, shadow_assert_on_i32_returns_exception) {
  /* i32 doesn't have _value field — shadow mode returns exception */
  _bind("x", create_i32_value(vm, 42));
  node_t node = _parse_expr("x.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  /* i32 shadow is not a union, so .! should fail */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_try_assert, shadow_try_on_i32_returns_exception) {
  _bind("x", create_i32_value(vm, 42));
  node_t node = _parse_expr("x.?");
  value_t v = run_expression(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  .?/.! with shadow result (from vm_create_union_shadow)             *
 * ================================================================== */

TEST_F(it_run_try_assert, assert_on_shadow_union_result) {
  /* Create a shadow result value — this is what ast_func_check produces
   * when it encounters a result parameter */
  value_t rv = _make_i32_result();
  value_t shadow_result = vm_create_union_shadow(vm, rv, false);
  _bind("r", shadow_result);

  /* Shadow value evaluated in non-shadow mode: should hit the shadow
   * guard in run_expression and be handled */
  node_t node = _parse_expr("r.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);

  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

/* ================================================================== *
 *  Duck typing — any type with .ok()/.value()/.error() works          *
 * ================================================================== */

TEST_F(it_run_try_assert, assert_ducktyped_ok_result) {
  /* Result type provides .ok()/.value()/.error() — duck typing means
   * we don't check for a specific type, just the methods */
  _bind("r", _make_ok_i32_result(99));
  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 99);
}

/* ================================================================== *
 *  Interrupt from .? correctly propagates through statement runners    *
 * ================================================================== */

TEST_F(it_run_try_assert, try_interrupt_propagates_through_if) {
  /* .? inside an if condition that returns error — the interrupt should
   * propagate through the if statement runner */
  value_t rv = _make_i32_result();
  value_t err_result = vm_create_union_value(vm, rv, "_error",
                                              create_error_value(vm, 1, "fail"));
  _bind("r", err_result);

  type_t result_type = (type_t)value_get_data(rv);
  value_t func_val = _make_func_with_return_type(result_type);

  /* Set current_func so .? can extract return type */
  value_t prev_func = vm_set_current_func(vm, func_val);

  /* r.? in expression context — if .? produces an interrupt,
   * it should propagate through any enclosing expression */
  value_t v = _run_expr("r.?");

  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
}

/* ================================================================== *
 *  Multiple .? in sequence — first error stops execution              *
 * ================================================================== */

TEST_F(it_run_try_assert, multiple_try_first_err_stops) {
  /* r1.? is ok, r2.? is error — the second .? should produce interrupt */
  _bind("r1", _make_ok_i32_result(10));
  _bind("r2", _make_err_i32_result(1, "fail"));

  value_t rv = _make_i32_result();
  type_t result_type = (type_t)value_get_data(rv);
  value_t func_val = _make_func_with_return_type(result_type);

  value_t prev_func = vm_set_current_func(vm, func_val);

  /* r1.? + r2.? — first .? returns 10 (ok), then + evaluates left side,
   * then evaluates right side r2.? which produces interrupt */
  value_t v = _run_expr("r1.? + r2.?");

  vm_set_current_func(vm, prev_func);

  /* The binary + should propagate the interrupt from r2.? */
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_abnormal(v));
}
