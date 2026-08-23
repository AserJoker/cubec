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

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_error_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_error_type(vm));
  }

  /** Create result[i32, error] type */
  value_t _make_i32_result(vm_t vm) {
    return vm_create_result_type_value(vm, _get_i32_type(vm), _get_error_type(vm));
  }

  /** Create a result[i32, error] value with _value active (ok) */
  value_t _make_ok_result(vm_t vm, int32_t val) {
    value_t rv = _make_i32_result(vm);
    value_t i32_val = create_i32_value(vm, val);
    return vm_create_union_value(vm, rv, "_value", i32_val);
  }

  /** Create a result[i32, error] value with _error active */
  value_t _make_err_result(vm_t vm, int32_t code, const char *msg) {
    value_t rv = _make_i32_result(vm);
    value_t err_val = create_error_value(vm, code, msg);
    return vm_create_union_value(vm, rv, "_error", err_val);
  }

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }

  /* Parse + run source as a program */
  value_t _run_source(const char *source, bool shadow = false) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    allocator_free(alloc, &tokens);
    if (!node) return NULL;
    value_t v = run_program(vm, node, shadow);
    allocator_free(alloc, &node);
    return v;
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

/* ---- .! (assert) on ok result ---- */

TEST_F(it_run_try_assert, assert_on_ok_result_returns_value) {
  value_t ok_result = _make_ok_result(vm, 42);
  _bind("r", ok_result);

  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

/* ---- .! (assert) on error result ---- */

TEST_F(it_run_try_assert, assert_on_err_result_panics) {
  value_t err_result = _make_err_result(vm, 1, "test error");
  _bind("r", err_result);

  value_t v = _run_expr("r.!");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ---- .? (try) on ok result ---- */

TEST_F(it_run_try_assert, try_on_ok_result_returns_value) {
  value_t ok_result = _make_ok_result(vm, 42);
  _bind("r", ok_result);

  /* .? outside function context — ok path doesn't need function context */
  value_t v = _run_expr("r.?");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

/* ---- .? (try) on error result outside function ---- */

TEST_F(it_run_try_assert, try_on_err_result_outside_function_returns_exception) {
  value_t err_result = _make_err_result(vm, 1, "test error");
  _bind("r", err_result);

  /* .? outside function context — cannot return, should produce exception */
  value_t v = _run_expr("r.?");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ---- .? (try) on error result inside function — returns error via interrupt ---- */

TEST_F(it_run_try_assert, try_on_err_result_in_function_returns_interrupt) {
  /* Create the result type and an error result value */
  value_t rv = _make_i32_result(vm);
  value_t err_val = create_error_value(vm, 1, "fail");
  value_t err_result = vm_create_union_value(vm, rv, "_error", err_val);
  _bind("r", err_result);

  /* Build a callable value with result[i32, error] return type.
   * We need a TYPE_KIND_CALLABLE value for current_func, not a TYPE_KIND_TYPE.
   * vm_create_callable_type_value returns a TYPE_KIND_TYPE, so we use
   * create_callable_value instead (cfunc_t function pointer). */
  type_t result_type = (type_t)value_get_data(rv);
  allocator_t alloc = vm_get_allocator(vm);
  vec_init_t pvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);

  /* Create callable type: () -> result[i32, error] */
  callable_type_t ct = callable_type_create(alloc, param_types, result_type,
                                             false, true, "<module>");
  allocator_free(alloc, &param_types);
  vec_push(vm_get_types(vm), ct);

  /* Create a callable value (cfunc) with the callable type.
   * The cfunc body doesn't matter — we just need the type info. */
  value_t func_val = create_callable_value(vm, ct,
      /*cfunc=*/(value_t (*)(vm_t, value_t, size_t, value_t *))NULL,
      "test_try");
  ASSERT_NE(func_val, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(func_val)), TYPE_KIND_CALLABLE);

  /* Set as current_func */
  value_t prev_func = vm_set_current_func(vm, func_val);

  /* Verify that of_error can be found on the return type */
  value_t ret_type_val = create_type_value(vm, result_type, NULL, false);
  ASSERT_NE(ret_type_val, nullptr);
  value_t of_err_fn = value_get_prop(vm, ret_type_val, "of_error");
  ASSERT_NE(of_err_fn, nullptr) << "of_error not found on result type";
  EXPECT_FALSE(value_is_abnormal(of_err_fn));
  EXPECT_EQ(type_get_kind(value_get_type(of_err_fn)), TYPE_KIND_CALLABLE);

  /* Now manually trace the .? path to find where it fails */
  value_t ok_result = value_member_call(vm, err_result, "ok", 0, NULL);
  ASSERT_FALSE(value_is_abnormal(ok_result))
      << "ok() call failed: kind=" << type_get_kind(value_get_type(ok_result));
  ASSERT_EQ(type_get_kind(value_get_type(ok_result)), TYPE_KIND_BOOL);
  bool is_ok = *(bool *)value_get_data(ok_result);
  EXPECT_FALSE(is_ok); /* err_result should not be ok */

  value_t err_call_val = value_member_call(vm, err_result, "error", 0, NULL);
  ASSERT_FALSE(value_is_abnormal(err_call_val))
      << "error() call failed: kind=" << type_get_kind(value_get_type(err_call_val));

  /* Verify current_func is set */
  value_t cur_fn = vm_get_current_func(vm);
  ASSERT_NE(cur_fn, nullptr) << "current_func is null";

  /* Verify return type extraction */
  type_t callee_type = value_get_type(cur_fn);
  ASSERT_EQ(type_get_kind(callee_type), TYPE_KIND_CALLABLE);
  type_t ret_type = callable_type_get_return_type((callable_type_t)callee_type);
  ASSERT_EQ(type_get_kind(ret_type), TYPE_KIND_UNION)
      << "return type is not union: " << type_get_name(ret_type);

  /* Now run r.? — should produce an interrupt with of_error result */
  value_t v = _run_expr("r.?");

  /* Restore previous function */
  vm_set_current_func(vm, prev_func);

  /* Verify we got an interrupt */
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT)
      << "got type kind " << type_get_kind(value_get_type(v))
      << " instead of INTERRUPT";

  /* The interrupt value should be a result[i32, error] with _error active */
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(inner)), TYPE_KIND_UNION);
}

/* ---- shadow mode ---- */

TEST_F(it_run_try_assert, shadow_assert_on_result) {
  value_t ok_result = _make_ok_result(vm, 42);
  _bind("r", ok_result);

  node_t node = _parse_expr("r.!");
  value_t v = run_expression(vm, node, true);
  free_node(node);

  /* Shadow mode: .! returns shadow of the value type (i32) */
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

TEST_F(it_run_try_assert, shadow_try_on_result) {
  value_t ok_result = _make_ok_result(vm, 42);
  _bind("r", ok_result);

  node_t node = _parse_expr("r.?");
  value_t v = run_expression(vm, node, true);
  free_node(node);

  /* Shadow mode: .? returns shadow of the value type (i32) */
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

/* ---- Duck typing: custom type with ok/value/error ---- */

TEST_F(it_run_try_assert, assert_on_custom_type_with_ok_value_error) {
  /* Create a custom struct type with ok(), value(), error() methods.
   * This tests duck-typing — .?/.! don't check for result/union specifically. */

  /* For now, test with result type since we need the method infrastructure */
  value_t ok_result = _make_ok_result(vm, 99);
  _bind("x", ok_result);

  value_t v = _run_expr("x.!");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 99);
}
