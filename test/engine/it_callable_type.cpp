#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/callable_type.h"
#include "engine/cfunc.h"
#include "engine/name.h"
#include "core/string.h"
#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_callable_type : public CubecTest {
protected:

  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_i64_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i64_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }

  /* Create a callable type via vm 鈥?registered in scope */
  callable_type_t _make_i32_to_i32_callable(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, _get_i32_type(vm));
    value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm), false, true, "<builtin>");
    allocator_free(alloc, &params);
    return (callable_type_t)value_get_data(tv);
  }

  callable_type_t _make_i32_bool_to_void_callable(vm_t vm) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, _get_i32_type(vm));
    vec_push(params, _get_bool_type(vm));
    value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm), false, true, "<builtin>");
    allocator_free(alloc, &params);
    return (callable_type_t)value_get_data(tv);
  }
};

/* ---- Native callback helpers ---- */

static value_t _add_one(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  int32_t val = *(int32_t *)value_get_data(argv[0]);
  int32_t result = val + 1;
  return vm_create_value(vm, (type_t)value_get_data(vm_get_i32_type(vm)), &result, NULL);
}

static value_t _return_true(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn; (void)argc; (void)argv;
  return create_bool_value(vm, true);
}

static value_t _echo_first(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn; (void)argc;
  return argv[0];
}

static value_t _noop(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn; (void)argc; (void)argv;
  return create_void_value(vm);
}

static value_t _variadic_sum(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  int32_t sum = 0;
  for (size_t i = 0; i < argc; i++) {
    sum += *(int32_t *)value_get_data(argv[i]);
  }
  return vm_create_value(vm, (type_t)value_get_data(vm_get_i32_type(vm)), &sum, NULL);
}

/* ---- Type creation ---- */

TEST_F(it_callable_type, create_basic) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);

  EXPECT_EQ(type_get_kind((type_t)ct), TYPE_KIND_CALLABLE);
  EXPECT_STREQ(type_get_name((type_t)ct), "(i32) -> i32");
  EXPECT_EQ(callable_type_get_param_count(ct), 1u);
  EXPECT_EQ(type_get_kind(callable_type_get_param_type(ct, 0)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(callable_type_get_return_type(ct)), TYPE_KIND_I32);
  EXPECT_FALSE(callable_type_is_variadic(ct));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, create_multi_param) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_bool_to_void_callable(vm);

  EXPECT_STREQ(type_get_name((type_t)ct), "(i32, bool) -> void");
  EXPECT_EQ(callable_type_get_param_count(ct), 2u);
  EXPECT_EQ(type_get_kind(callable_type_get_return_type(ct)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, create_variadic) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(params, _get_i32_type(vm));
  value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm), true, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  EXPECT_STREQ(type_get_name((type_t)ct), "(i32, ...) -> i32");
  EXPECT_TRUE(callable_type_is_variadic(ct));
  EXPECT_EQ(callable_type_get_param_count(ct), 1u);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, create_no_params) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_bool_type(vm), false, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  EXPECT_STREQ(type_get_name((type_t)ct), "() -> bool");
  EXPECT_EQ(callable_type_get_param_count(ct), 0u);

  vm_dispose(vm, allocator);
}

/* ---- Value creation ---- */

TEST_F(it_callable_type, create_callable_value) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);

  EXPECT_NE(cv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cv)), TYPE_KIND_CALLABLE);
  EXPECT_NE(value_get_data(cv), nullptr);
  EXPECT_FALSE(value_is_own(cv)); /* data is borrowed cfunc_t, scope->cfuncs owns it */
  EXPECT_TRUE(value_is_initialized(cv));

  /* cfunc_t registered in scope (+1 for builtin panic) */
  scope_t scope = vm_get_current_scope(vm);
  EXPECT_EQ(vec_get_size(scope->cfuncs), 2u);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, create_callable_shadow) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_shadow(vm, ct, true);

  EXPECT_TRUE(value_is_shadow(cv));
  EXPECT_TRUE(value_is_initialized(cv));

  vm_dispose(vm, allocator);
}

/* ---- Call ---- */

TEST_F(it_callable_type, call_basic) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);

  int32_t arg = 41;
  value_t argv[1] = { vm_create_value(vm, _get_i32_type(vm), &arg, NULL) };
  value_t result = value_call(vm, cv, 1, argv);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 42);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, call_no_args) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_bool_type(vm), false, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t cv = create_callable_value(vm, ct, _return_true, NULL);
  value_t result = value_call(vm, cv, 0, NULL);

  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, call_void_return) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_bool_to_void_callable(vm);
  value_t cv = create_callable_value(vm, ct, _noop, NULL);

  int32_t a = 1;
  bool b = true;
  value_t argv[2] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    create_bool_value(vm, b),
  };
  value_t result = value_call(vm, cv, 2, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, call_wrong_argc) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);

  value_t result = value_call(vm, cv, 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, call_not_callable_type) {
  vm_t vm = vm_create(allocator);
  value_t not_callable = create_bool_value(vm, true);
  value_t result = value_call(vm, not_callable, 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- Call with safe_cast ---- */

TEST_F(it_callable_type, call_safe_cast_arg) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _echo_first, NULL);

  /* Pass i64 to i32 parameter — narrowing safe_cast is rejected */
  int64_t big = 42;
  value_t argv[1] = { vm_create_value(vm, _get_i64_type(vm), &big, NULL) };
  value_t result = value_call(vm, cv, 1, argv);

  /* safe_cast i64→i32 is narrowing, returns exception */
  EXPECT_TRUE(value_is_error(result));

  vm_dispose(vm, allocator);
}

/* ---- Variadic call ---- */

TEST_F(it_callable_type, call_variadic) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(params, _get_i32_type(vm));
  value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm), true, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t cv = create_callable_value(vm, ct, _variadic_sum, NULL);

  int32_t a = 10, b = 20, c = 30;
  value_t argv[3] = {
    vm_create_value(vm, _get_i32_type(vm), &a, NULL),
    vm_create_value(vm, _get_i32_type(vm), &b, NULL),
    vm_create_value(vm, _get_i32_type(vm), &c, NULL),
  };
  value_t result = value_call(vm, cv, 3, argv);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 60);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, call_variadic_too_few) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(params, _get_i32_type(vm));
  value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm), true, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t cv = create_callable_value(vm, ct, _variadic_sum, NULL);

  /* 0 args but need at least 1 */
  value_t result = value_call(vm, cv, 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

/* ---- type_equal ---- */

TEST_F(it_callable_type, type_equal_same) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct1 = _make_i32_to_i32_callable(vm);
  callable_type_t ct2 = _make_i32_to_i32_callable(vm);

  vtable_t vt = type_get_vtable((type_t)ct1);
  value_t eq = vt.type_equal(vm, (type_t)ct1, (type_t)ct2);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, type_equal_different_params) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct1 = _make_i32_to_i32_callable(vm);
  callable_type_t ct2 = _make_i32_bool_to_void_callable(vm);

  vtable_t vt = type_get_vtable((type_t)ct1);
  value_t eq = vt.type_equal(vm, (type_t)ct1, (type_t)ct2);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, type_equal_variadic_vs_nonvariadic) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* non-variadic: (i32) -> i32 */
  callable_type_t ct_nv = _make_i32_to_i32_callable(vm);

  /* variadic: (i32, ...) -> i32 */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  vec_push(params, _get_i32_type(vm));
  value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm), true, true, "<builtin>");
  allocator_free(alloc, &params);
  callable_type_t ct_v = (callable_type_t)value_get_data(tv);

  vtable_t vt = type_get_vtable((type_t)ct_nv);
  value_t eq = vt.type_equal(vm, (type_t)ct_nv, (type_t)ct_v);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, type_extends_wildcard) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  type_t wc = (type_t)value_get_data(vm_get_wildcard_type(vm));

  vtable_t vt = type_get_vtable((type_t)ct);
  value_t ext = vt.type_extends(vm, (type_t)ct, wc);
  EXPECT_TRUE(*(bool *)value_get_data(ext));

  vm_dispose(vm, allocator);
}

/* ---- clone ---- */

TEST_F(it_callable_type, clone) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);

  value_t cloned = value_clone(vm, cv);
  EXPECT_NE(cloned, nullptr);
  EXPECT_NE(cloned, cv);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_CALLABLE);

  /* cloned has same func pointer */
  cfunc_t orig_fc = (cfunc_t)value_get_data(cv);
  cfunc_t clone_fc = (cfunc_t)value_get_data(cloned);
  EXPECT_EQ(orig_fc->func, clone_fc->func);

  vm_dispose(vm, allocator);
}

/* ---- equal ---- */

TEST_F(it_callable_type, equal_same_func) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t a = create_callable_value(vm, ct, _add_one, NULL);
  value_t b = create_callable_value(vm, ct, _add_one, NULL);

  value_t eq = value_equal(vm, a, b);
  EXPECT_TRUE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, equal_different_func) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t a = create_callable_value(vm, ct, _add_one, NULL);
  value_t b = create_callable_value(vm, ct, _echo_first, NULL);

  value_t eq = value_equal(vm, a, b);
  EXPECT_FALSE(*(bool *)value_get_data(eq));

  vm_dispose(vm, allocator);
}

/* ---- assignment ---- */

TEST_F(it_callable_type, assignment) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t a = create_callable_value(vm, ct, _add_one, NULL);
  value_t b = create_callable_value(vm, ct, _echo_first, NULL);

  value_t result = value_assignment(vm, b, a);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  /* b now has _add_one func pointer */
  cfunc_t fa = (cfunc_t)value_get_data(a);
  cfunc_t fb = (cfunc_t)value_get_data(b);
  EXPECT_EQ(fa->func, fb->func);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, assignment_shadow) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t a = create_callable_value(vm, ct, _add_one, NULL);
  value_t b = create_callable_shadow(vm, ct, false);

  /* assigning to shadow just marks initialized, no data copy */
  value_t result = value_assignment(vm, b, a);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);
  EXPECT_TRUE(value_is_initialized(b));
  EXPECT_TRUE(value_is_shadow(b)); /* shadow stays shadow */

  vm_dispose(vm, allocator);
}

/* ---- to_string ---- */

TEST_F(it_callable_type, to_string) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);

  value_t str = value_to_string(vm, cv);
  EXPECT_EQ(type_get_kind(value_get_type(str)), TYPE_KIND_STR);

  vm_dispose(vm, allocator);
}

/* ---- self-inspection via fn parameter ---- */

static value_t _inspect_self(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc; (void)argv;
  /* fn should be the callable value itself */
  EXPECT_NE(fn, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(fn)), TYPE_KIND_CALLABLE);
  return create_bool_value(vm, true);
}

TEST_F(it_callable_type, call_fn_self_inspection) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _inspect_self, NULL);

  int32_t arg = 0;
  value_t argv[1] = { vm_create_value(vm, _get_i32_type(vm), &arg, NULL) };
  value_t result = value_call(vm, cv, 1, argv);
  EXPECT_TRUE(*(bool *)value_get_data(result));

  vm_dispose(vm, allocator);
}

/* ---- Shadow operations ---- */

TEST_F(it_callable_type, shadow_equal) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t a = create_callable_shadow(vm, ct, true);
  value_t b = create_callable_shadow(vm, ct, true);
  value_t result = value_equal(vm, a, b);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, shadow_call) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_shadow(vm, ct, true);
  int32_t arg = 42;
  value_t argv[1] = { vm_create_value(vm, _get_i32_type(vm), &arg, NULL) };
  value_t result = value_call(vm, cv, 1, argv);
  /* shadow call returns shadow of return type */
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, shadow_safe_cast) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_shadow(vm, ct, true);
  value_t result = value_safe_cast(vm, cv, (type_t)ct);
  EXPECT_TRUE(value_is_shadow(result));
  EXPECT_EQ(value_get_type(result), (type_t)ct);
  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, shadow_to_string) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_shadow(vm, ct, true);
  value_t result = value_to_string(vm, cv);
  EXPECT_TRUE(value_is_shadow(result));
  vm_dispose(vm, allocator);
}

/* ---- Closure scope ---- */

static value_t _read_captured(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc; (void)argv;
  /* The callback retrieves its own closure scope and looks up captured vars.
   * callable does NOT push/pop the closure scope — it's the callback's job. */
  cfunc_t fc = (cfunc_t)value_get_data(fn);
  scope_t closure = cfunc_get_closure_scope(fc);
  if (!closure)
    return create_exception_value(vm, "closure: no closure scope");
  name_t n = scope_lookup(closure, "captured");
  if (!n || !n->ref)
    return create_exception_value(vm, "closure: 'captured' not found");
  return n->ref;
}

TEST_F(it_callable_type, closure_scope_create_and_dispose) {
  /* Create closure scope with a captured value, then vm_dispose should handle it */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, "test_fn");

  /* Create a value in current scope, then capture it */
  int32_t val = 42;
  vm_create_value(vm, _get_i32_type(vm), &val, "captured");
  value_t result = callable_capture(vm, cv, "captured");
  EXPECT_FALSE(value_is_error(result));

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, closure_scope_accessible_during_call) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  callable_type_t ct = _make_i32_to_i32_callable(vm);

  /* Create a callable that reads "captured" from its closure scope */
  value_t cv = create_callable_value(vm, ct, _read_captured, "clojure");

  /* Create a value in current scope, then capture it into the callable */
  int32_t val = 42;
  vm_create_value(vm, _get_i32_type(vm), &val, "captured");
  callable_capture(vm, cv, "captured");

  /* Call the callable — callback reads "captured" from its own closure scope */
  int32_t arg = 0;
  value_t argv[1] = { vm_create_value(vm, _get_i32_type(vm), &arg, NULL) };
  value_t result = value_call(vm, cv, 1, argv);
  EXPECT_FALSE(value_is_error(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 42);

  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, closure_scope_null_for_plain_cfunc) {
  vm_t vm = vm_create(allocator);
  callable_type_t ct = _make_i32_to_i32_callable(vm);
  value_t cv = create_callable_value(vm, ct, _add_one, NULL);
  cfunc_t fc = (cfunc_t)value_get_data(cv);
  /* Plain C functions have no closure scope */
  EXPECT_EQ(cfunc_get_closure_scope(fc), nullptr);
  vm_dispose(vm, allocator);
}

TEST_F(it_callable_type, clone_has_no_closure) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  callable_type_t ct = _make_i32_to_i32_callable(vm);

  value_t cv = create_callable_value(vm, ct, _add_one, "original");
  /* Capture a variable so the callable has a closure scope */
  int32_t val = 99;
  vm_create_value(vm, _get_i32_type(vm), &val, "x");
  callable_capture(vm, cv, "x");

  /* Clone should share func+name but have no closure (closures are unique) */
  value_t cloned = value_clone(vm, cv);
  cfunc_t orig_fc = (cfunc_t)value_get_data(cv);
  cfunc_t clone_fc = (cfunc_t)value_get_data(cloned);
  EXPECT_EQ(orig_fc->func, clone_fc->func);
  EXPECT_EQ(cfunc_get_closure_scope(clone_fc), nullptr);

  vm_dispose(vm, allocator);
}
