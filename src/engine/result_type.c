/**
 * @file result_type.c
 * @brief result[T,E] built-in type — tagged union with _value:T, _error:E
 *        and convenience methods: ok, value, error, of_value, of_error.
 */
#include "engine/result_type.h"
#include "engine/vm.h"
#include "engine/callable_type.h"
#include "engine/pointer_type.h"
#include "engine/union_type.h"
#include "engine/struct_type.h"
#include "engine/bool_type.h"
#include "engine/error.h"
#include "engine/exception_type.h"
#include "engine/scope.h"
#include "core/allocator.h"
#include "core/class.h"
#include "core/vec.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Result method C functions                                           */
/* ================================================================== */

/** ok(*const result[T,E]) -> bool
 *  Returns true if the active variant is _value (tag=0). */
static value_t _result_ok(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.ok expected 1 argument");
  /* argv[0] = *const result[T,E] (pointer from member_call) */
  value_t self = value_deref_get(vm, argv[0]);
  if (value_is_abnormal(self))
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(_union_type_find_field(ut, "_value"));
  return value_is(vm, self, T);
}

/** value(*const result[T,E]) -> T
 *  Returns _value if ok, panics otherwise. */
static value_t _result_value(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.value expected 1 argument");
  value_t self = value_deref_get(vm, argv[0]);
  if (value_is_abnormal(self))
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(_union_type_find_field(ut, "_value"));
  value_t is_ok = value_is(vm, self, T);
  if (value_is_abnormal(is_ok))
    return is_ok;
  if (!*(bool *)value_get_data(is_ok))
    return create_exception_value(vm, "result.value called on error variant");
  /* read _value directly from payload via get_field_raw (bypasses result wrapping) */
  return value_get_field_raw(vm, self, "_value");
}

/** error(*const result[T,E]) -> E
 *  Returns _error if !ok, panics otherwise. */
static value_t _result_error(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.error expected 1 argument");
  value_t self = value_deref_get(vm, argv[0]);
  if (value_is_abnormal(self))
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(_union_type_find_field(ut, "_value"));
  value_t is_ok = value_is(vm, self, T);
  if (value_is_abnormal(is_ok))
    return is_ok;
  if (*(bool *)value_get_data(is_ok))
    return create_exception_value(vm, "result.error called on ok variant");
  /* read _error directly from payload via get_field_raw (bypasses result wrapping) */
  return value_get_field_raw(vm, self, "_error");
}

/** of_value(T) -> result[T,E]  (static constructor)
 *  Uses return type of callable to recover union_type_t. */
static value_t _result_of_value(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.of_value expected 1 argument");
  callable_type_t ct = (callable_type_t)value_get_type(fn);
  union_type_t ut = (union_type_t)callable_type_get_return_type(ct);
  type_t T = field_info_get_type(_union_type_find_field(ut, "_value"));
  value_t val = value_safe_cast(vm, argv[0], T);
  if (value_is_abnormal(val))
    return val;
  return _union_type_create_value(vm, ut, 0, val);
}

/** of_error(E) -> result[T,E]  (static constructor)
 *  Uses return type of callable to recover union_type_t. */
static value_t _result_of_error(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.of_error expected 1 argument");
  callable_type_t ct = (callable_type_t)value_get_type(fn);
  union_type_t ut = (union_type_t)callable_type_get_return_type(ct);
  type_t E = field_info_get_type(_union_type_find_field(ut, "_error"));
  value_t err = value_safe_cast(vm, argv[0], E);
  if (value_is_abnormal(err))
    return err;
  return _union_type_create_value(vm, ut, 1, err);
}

/* ================================================================== */
/* vm_create_result_type_value                                         */
/* ================================================================== */

value_t vm_create_result_type_value(vm_t vm, type_t T, type_t E) {
  allocator_t alloc = vm_get_allocator(vm);
  const char *module_id = vm_get_current_module_id(vm);

  /* generate name: result[T_name,E_name] */
  const char *tname = type_get_name(T);
  const char *ename = type_get_name(E);
  size_t name_len = 7 + strlen(tname) + 1 + strlen(ename) + 1; /* "result[,,,]" */
  char *name = (char *)allocator_alloc(alloc, name_len + 1);
  snprintf(name, name_len + 1, "result[%s,%s]", tname, ename);

  /* create union type value via vm API */
  value_t type_val = vm_create_union_type_value(vm, name, true, module_id);
  allocator_free(alloc, &name); /* vm_create_union_type_value cloned it */

  /* Push a temporary child scope so void values from add_field/seal/add_prop
   * land there instead of polluting the caller's scope.
   * Props are cloned into the union's own scope (owned), so the temporary
   * scope can be safely disposed after construction. */
  scope_t bootstrap_scope = scope_create(alloc, SCOPE_BLOCK,
                                         vm_get_current_scope(vm), NULL);
  vm_push_scope(vm, bootstrap_scope);

  /* add _value:T, _error:E fields via vm_union_add_field */
  type_t type_type_val = (type_t)value_get_data(vm_get_type_type(vm));
  {
    value_t tmp = value_create(alloc, type_type_val, T, false);
    (void)vm_union_add_field(vm, type_val, "_value", tmp, false); /* private */
    allocator_free(alloc, &tmp);
  }
  {
    value_t tmp = value_create(alloc, type_type_val, E, false);
    (void)vm_union_add_field(vm, type_val, "_error", tmp, false); /* private */
    allocator_free(alloc, &tmp);
  }

  /* seal the union type */
  value_t seal_err = vm_union_seal(vm, type_val);
  if (value_is_abnormal(seal_err)) {
    vm_pop_scope(vm);
    scope_dispose(bootstrap_scope);
    return seal_err;
  }

  /* extract union_type_t for method registration (read-only access) */
  union_type_t ut = (union_type_t)value_get_data(type_val);

  /* ---- Register methods ---- */

  /* *const result[T,E] type — used as self parameter for instance methods */
  pointer_type_t const_result_ptr = pointer_type_create(alloc, (type_t)ut, false, false);
  vec_push(bootstrap_scope->types, const_result_ptr);

  type_t bool_t = (type_t)value_get_data(vm_get_bool_type(vm));

  /* ok(*const result) -> bool */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, bool_t, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    vec_push(bootstrap_scope->types, ct);
    value_t fn = create_callable_value(vm, ct, _result_ok, "ok");
    (void)vm_union_add_prop(vm, type_val, "ok", fn, true, true);
  }

  /* value(*const result) -> T */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, T, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    vec_push(bootstrap_scope->types, ct);
    value_t fn = create_callable_value(vm, ct, _result_value, "value");
    (void)vm_union_add_prop(vm, type_val, "value", fn, true, true);
  }

  /* error(*const result) -> E */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, E, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    vec_push(bootstrap_scope->types, ct);
    value_t fn = create_callable_value(vm, ct, _result_error, "error");
    (void)vm_union_add_prop(vm, type_val, "error", fn, true, true);
  }

  /* of_value(T) -> result[T,E]  (static, not method) */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, T);
    callable_type_t ct = callable_type_create(alloc, params, (type_t)ut, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    vec_push(bootstrap_scope->types, ct);
    value_t fn = create_callable_value(vm, ct, _result_of_value, "of_value");
    (void)vm_union_add_prop(vm, type_val, "of_value", fn, false, true);
  }

  /* of_error(E) -> result[T,E]  (static, not method) */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, E);
    callable_type_t ct = callable_type_create(alloc, params, (type_t)ut, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    vec_push(bootstrap_scope->types, ct);
    value_t fn = create_callable_value(vm, ct, _result_of_error, "of_error");
    (void)vm_union_add_prop(vm, type_val, "of_error", fn, false, true);
  }

  /* Pop and dispose bootstrap scope — void values and original callable values
   * are freed, but union's props hold clones (owned by union's scope). */
  vm_pop_scope(vm);
  scope_dispose(bootstrap_scope);

  return type_val;
}
