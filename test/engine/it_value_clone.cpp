#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/error_type.h"
#include "engine/scope.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_value_clone : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
};

/* ---- Basic clone per type ---- */

TEST_F(it_value_clone, bool_clone) {
  vm_t vm = vm_create(allocator);
  value_t a = create_bool_value(vm, true);
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_BOOL);
  EXPECT_TRUE(value_is_own(c));
  EXPECT_TRUE(value_is_initialized(c));
  EXPECT_EQ(*(bool *)value_get_data(c), true);

  /* original unchanged */
  EXPECT_EQ(*(bool *)value_get_data(a), true);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, i32_clone) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 42);
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_own(c));
  EXPECT_EQ(*(int32_t *)value_get_data(c), 42);
  EXPECT_NE(value_get_data(c), value_get_data(a));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, str_clone) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "hello");
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_STR);
  EXPECT_TRUE(value_is_own(c));
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(c)), "hello");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, void_clone) {
  vm_t vm = vm_create(allocator);
  value_t a = create_void_value(vm);
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_VOID);
  EXPECT_EQ(value_get_data(c), nullptr);
  EXPECT_TRUE(value_is_initialized(c));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, error_clone) {
  vm_t vm = vm_create(allocator);
  value_t a = create_error_value(vm, "test error %d", 42);
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_ERROR);
  struct error_data_t *d = (struct error_data_t *)value_get_data(c);
  EXPECT_STREQ(d->message, "test error 42");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, shadow_clone) {
  vm_t vm = vm_create(allocator);
  type_t i32t = _get_i32_type(vm);
  value_t a = vm_create_value_shadow(vm, i32t, NULL, false);
  value_t c = value_clone(vm, a);

  EXPECT_NE(c, a);
  EXPECT_TRUE(value_is_shadow(c));
  EXPECT_EQ(type_get_kind(value_get_type(c)), TYPE_KIND_I32);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Clone independent data: modify clone does not affect original ---- */

TEST_F(it_value_clone, i32_clone_independence) {
  vm_t vm = vm_create(allocator);
  value_t a = create_i32_value(vm, 10);
  value_t c = value_clone(vm, a);

  /* modify clone's data */
  *(int32_t *)value_get_data(c) = 99;
  EXPECT_EQ(*(int32_t *)value_get_data(a), 10);
  EXPECT_EQ(*(int32_t *)value_get_data(c), 99);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, str_clone_independence) {
  vm_t vm = vm_create(allocator);
  value_t a = create_str_value(vm, "original");
  value_t c = value_clone(vm, a);

  /* assign to clone, original unchanged */
  value_t new_val = create_str_value(vm, "modified");
  value_assignment(vm, c, new_val);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(a)), "original");
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(c)), "modified");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-scope: function argument passing (caller → callee) ---- */

TEST_F(it_value_clone, function_arg_i32) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* caller scope: create argument */
  scope_t caller = vm_get_current_scope(vm);
  value_t arg = create_i32_value(vm, 42);

  /* switch to callee scope (independent scope tree) */
  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  /* clone argument into callee scope */
  value_t local = value_clone(vm, arg);
  EXPECT_NE(local, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(local)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(local), 42);
  /* local is registered in callee scope */
  EXPECT_EQ(vec_get_size(callee->values), 1u);

  /* restore caller scope */
  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  /* original still intact */
  EXPECT_EQ(*(int32_t *)value_get_data(arg), 42);

  /* callee scope cleanup */
  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, function_arg_str) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t caller = vm_get_current_scope(vm);
  value_t arg = create_str_value(vm, "hello");

  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t local = value_clone(vm, arg);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(local)), "hello");
  /* cloned string_t is in callee->strings */
  EXPECT_EQ(vec_get_size(callee->strings), 1u);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  EXPECT_STREQ(string_get(*(string_t *)value_get_data(arg)), "hello");

  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, function_arg_bool) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t caller = vm_get_current_scope(vm);
  value_t arg = create_bool_value(vm, true);

  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t local = value_clone(vm, arg);
  EXPECT_EQ(*(bool *)value_get_data(local), true);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-scope: return value (callee → caller) ---- */

TEST_F(it_value_clone, return_value_i32) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* callee scope: compute result */
  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t result = create_i32_value(vm, 100);

  /* switch back to caller */
  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  /* clone return value into caller scope */
  value_t ret = value_clone(vm, result);
  EXPECT_EQ(*(int32_t *)value_get_data(ret), 100);
  /* ret is in caller scope */
  scope_t caller = vm_get_current_scope(vm);
  EXPECT_GT(vec_get_size(caller->values), 0u);

  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, return_value_str) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t callee = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, callee);
  scope_t prev_root = vm_set_root_scope(vm, callee);

  value_t result = create_str_value(vm, "result");

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  value_t ret = value_clone(vm, result);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(ret)), "result");
  /* cloned string_t in caller scope */
  scope_t caller = vm_get_current_scope(vm);
  EXPECT_EQ(vec_get_size(caller->strings), 1u);

  allocator_free(alloc, &callee);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Cross-scope: closure capture (outer → closure scope) ---- */

TEST_F(it_value_clone, closure_capture_i32) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* outer scope: define captured variable */
  scope_t outer = vm_get_current_scope(vm);
  value_t captured = create_i32_value(vm, 77);

  /* closure scope: independent scope tree */
  scope_t closure_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, closure_scope);
  scope_t prev_root = vm_set_root_scope(vm, closure_scope);

  /* clone captured value into closure scope */
  value_t local_copy = value_clone(vm, captured);
  EXPECT_EQ(*(int32_t *)value_get_data(local_copy), 77);
  EXPECT_EQ(vec_get_size(closure_scope->values), 1u);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  /* outer value still intact */
  EXPECT_EQ(*(int32_t *)value_get_data(captured), 77);

  allocator_free(alloc, &closure_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, closure_capture_str) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t outer = vm_get_current_scope(vm);
  value_t captured = create_str_value(vm, "captured");

  scope_t closure_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, closure_scope);
  scope_t prev_root = vm_set_root_scope(vm, closure_scope);

  value_t local_copy = value_clone(vm, captured);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(local_copy)), "captured");
  EXPECT_EQ(vec_get_size(closure_scope->strings), 1u);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  EXPECT_STREQ(string_get(*(string_t *)value_get_data(captured)), "captured");

  allocator_free(alloc, &closure_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, closure_capture_multiple) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* outer scope: multiple captured variables */
  scope_t outer = vm_get_current_scope(vm);
  value_t x = create_i32_value(vm, 1);
  value_t y = create_bool_value(vm, false);
  value_t s = create_str_value(vm, "msg");

  /* closure scope */
  scope_t closure_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, closure_scope);
  scope_t prev_root = vm_set_root_scope(vm, closure_scope);

  value_t lx = value_clone(vm, x);
  value_t ly = value_clone(vm, y);
  value_t ls = value_clone(vm, s);

  EXPECT_EQ(*(int32_t *)value_get_data(lx), 1);
  EXPECT_EQ(*(bool *)value_get_data(ly), false);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(ls)), "msg");
  EXPECT_EQ(vec_get_size(closure_scope->values), 3u);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  allocator_free(alloc, &closure_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Closure capture: original modified after capture does not affect clone ---- */

TEST_F(it_value_clone, closure_capture_isolation) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t outer = vm_get_current_scope(vm);
  value_t x = create_i32_value(vm, 10);

  scope_t closure_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, closure_scope);
  scope_t prev_root = vm_set_root_scope(vm, closure_scope);

  value_t lx = value_clone(vm, x);

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  /* modify original after capture */
  *(int32_t *)value_get_data(x) = 99;

  /* closure's clone is unaffected */
  EXPECT_EQ(*(int32_t *)value_get_data(lx), 10);

  allocator_free(alloc, &closure_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Function call: full round-trip (arg in → return out) ---- */

TEST_F(it_value_clone, function_call_roundtrip) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* caller scope: prepare argument */
  scope_t caller = vm_get_current_scope(vm);
  value_t arg = create_i32_value(vm, 5);

  /* --- enter function scope --- */
  scope_t func_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, func_scope);
  scope_t prev_root = vm_set_root_scope(vm, func_scope);

  /* clone arg into function scope */
  value_t param = value_clone(vm, arg);
  EXPECT_EQ(*(int32_t *)value_get_data(param), 5);

  /* compute result in function scope */
  value_t result = create_i32_value(vm, *(int32_t *)value_get_data(param) * 2);

  /* --- exit function scope --- */
  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  /* clone return value into caller scope */
  value_t ret = value_clone(vm, result);
  EXPECT_EQ(*(int32_t *)value_get_data(ret), 10);

  allocator_free(alloc, &func_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, function_call_str_roundtrip) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t caller = vm_get_current_scope(vm);
  value_t arg = create_str_value(vm, "hello");

  scope_t func_scope = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_scope = vm_set_scope(vm, func_scope);
  scope_t prev_root = vm_set_root_scope(vm, func_scope);

  value_t param = value_clone(vm, arg);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(param)), "hello");

  /* concatenate in function */
  value_t extra = create_str_value(vm, " world");
  value_t result = value_add(vm, param, extra);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(result)), "hello world");

  vm_set_scope(vm, prev_scope);
  vm_set_root_scope(vm, prev_root);

  value_t ret = value_clone(vm, result);
  EXPECT_STREQ(string_get(*(string_t *)value_get_data(ret)), "hello world");

  allocator_free(alloc, &func_scope);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Nested function call (scope A → scope B → scope C) ---- */

TEST_F(it_value_clone, nested_function_call) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* scope A: original value */
  scope_t scope_a = vm_get_current_scope(vm);
  value_t orig = create_i32_value(vm, 7);

  /* scope B: first function */
  scope_t scope_b = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_b = vm_set_scope(vm, scope_b);
  scope_t prev_root_b = vm_set_root_scope(vm, scope_b);

  value_t param_b = value_clone(vm, orig);

  /* scope C: nested function */
  scope_t scope_c = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
  scope_t prev_c = vm_set_scope(vm, scope_c);
  scope_t prev_root_c = vm_set_root_scope(vm, scope_c);

  value_t param_c = value_clone(vm, param_b);
  EXPECT_EQ(*(int32_t *)value_get_data(param_c), 7);
  EXPECT_EQ(vec_get_size(scope_c->values), 1u);

  /* return from C */
  value_t result_c = create_i32_value(vm, *(int32_t *)value_get_data(param_c) + 3);

  vm_set_scope(vm, prev_c);
  vm_set_root_scope(vm, prev_root_c);

  /* return from B */
  value_t result_b = value_clone(vm, result_c);

  vm_set_scope(vm, prev_b);
  vm_set_root_scope(vm, prev_root_b);

  /* back in A */
  value_t final = value_clone(vm, result_b);
  EXPECT_EQ(*(int32_t *)value_get_data(final), 10);

  allocator_free(alloc, &scope_c);
  allocator_free(alloc, &scope_b);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- vm_set_scope / vm_set_root_scope API tests ---- */

TEST_F(it_value_clone, vm_set_scope_returns_previous) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t global = vm_get_current_scope(vm);
  scope_t func = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);

  scope_t prev = vm_set_scope(vm, func);
  EXPECT_EQ(prev, global);
  EXPECT_EQ(vm_get_current_scope(vm), func);

  scope_t prev2 = vm_set_scope(vm, prev);
  EXPECT_EQ(prev2, func);
  EXPECT_EQ(vm_get_current_scope(vm), global);

  allocator_free(alloc, &func);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value_clone, vm_set_root_scope_returns_previous) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  scope_t global = vm_get_current_scope(vm);
  scope_t func = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);

  vm_set_root_scope(vm, global); /* ensure root is global */
  scope_t prev = vm_set_root_scope(vm, func);
  EXPECT_EQ(prev, global);
  EXPECT_EQ(vm_get_root_scope(vm), func);

  vm_set_root_scope(vm, prev);
  EXPECT_EQ(vm_get_root_scope(vm), global);

  allocator_free(alloc, &func);
  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Scope cleanup: cloned values live in their scope ---- */

TEST_F(it_value_clone, cloned_values_disposed_with_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  size_t before = vec_get_size(vm_get_current_scope(vm)->values);

  {
    scope_t func = scope_create(alloc, SCOPE_FUNCTION, NULL, NULL);
    scope_t prev = vm_set_scope(vm, func);
    scope_t prev_root = vm_set_root_scope(vm, func);

    /* clone creates values in func scope */
    value_t a = create_i32_value(vm, 1);
    value_t b = create_str_value(vm, "test");
    value_t c = create_bool_value(vm, true);
    (void)a; (void)b; (void)c;

    vm_set_scope(vm, prev);
    vm_set_root_scope(vm, prev_root);

    /* disposing func scope frees all cloned values */
    allocator_free(alloc, &func);
  }

  /* caller scope values unchanged */
  size_t after = vec_get_size(vm_get_current_scope(vm)->values);
  EXPECT_EQ(before, after);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
