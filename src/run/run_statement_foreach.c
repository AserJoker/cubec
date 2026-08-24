#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/callable_type.h"
#include "engine/struct_type.h"
#include "cubec/node.h"
#include "cubec/statement_foreach.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"

/* ---- helpers ---- */

static const char *_get_var_name(node_t variable) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)variable;
  return string_get(id->value);
}

static void _bind_or_update_name(vm_t vm, value_t val, const char *name,
                                 bool is_var_decl) {
  scope_t scope = vm_get_current_scope(vm);
  if (!scope || !name) return;

  struct _name_t *existing = scope_lookup(scope, name);
  if (existing && !is_var_decl) {
    /* lvalue mode: update existing binding */
    existing->ref = val;
  } else {
    /* var mode or new name: create binding in current scope */
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }
}

/**
 * @brief Look up a method on a struct type value and return its return type.
 * Returns NULL if the method is not found.
 */
static type_t _get_method_return_type(vm_t vm, value_t type_val,
                                       const char *method_name) {
  strmap_t methods = vm_struct_get_methods(vm, type_val);
  if (!methods) return NULL;
  value_t method = (value_t)strmap_find(methods, method_name);
  if (!method) return NULL;
  callable_type_t ct = (callable_type_t)value_get_type(method);
  return ct->return_type;
}

value_t run_statement_foreach(vm_t vm, node_t node, bool shadow) {
  cubec_statement_foreach_t stmt = (cubec_statement_foreach_t)node;
  const char *var_name = _get_var_name(stmt->variable);
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));

  /* ---- Evaluate iterator expression ---- */
  value_t iter_val = run_expression(vm, stmt->iterator, shadow);
  if (value_is_interrupt(iter_val)) return iter_val;
  if (value_is_abnormal(iter_val)) return iter_val;

  /* ---- Shadow mode ---- */
  if (shadow || value_is_shadow(iter_val)) {
    /* Type-level analysis: derive types from method signatures without
     * executing actual method calls. Shadow mode should never invoke
     * C callback functions. */

    type_t iter_type = value_get_type(iter_val);

    /* Get __iter__ return type (the iterator type) */
    if (type_get_kind(iter_type) != TYPE_KIND_STRUCT) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach requires a struct with __iter__ method");
      return create_void_value(vm);
    }

    /* Find the type value for iter_type to look up methods */
    value_t iter_type_val = vm_create_value(vm, iter_type, NULL, NULL);

    type_t iterator_type = _get_method_return_type(vm, iter_type_val, "__iter__");
    if (!iterator_type) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach iterator has no __iter__ method");
      return create_void_value(vm);
    }

    /* Get next() return type (the result type with done/value) */
    value_t iterator_type_val = vm_create_value(vm, iterator_type, NULL, NULL);
    type_t result_type = _get_method_return_type(vm, iterator_type_val, "next");
    if (!result_type) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach iterator has no next method");
      return create_void_value(vm);
    }

    /* Result type should have .done (bool) and .value fields */
    if (type_get_kind(result_type) != TYPE_KIND_STRUCT) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach iterator result must be a struct");
      return create_void_value(vm);
    }

    value_t result_type_val = vm_create_value(vm, result_type, NULL, NULL);
    if (!vm_struct_find_field(vm, result_type_val, "done")) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach iterator result has no 'done' field");
      return create_void_value(vm);
    }
    if (!vm_struct_find_field(vm, result_type_val, "value")) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "foreach iterator result has no 'value' field");
      return create_void_value(vm);
    }

    /* Evaluate body once for type checking */
    value_t body_val = run_statement(vm, stmt->body, true);
    if (value_is_abnormal(body_val) && !value_is_interrupt(body_val))
      return body_val;
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      /* BREAK / CONTINUE consumed by loop */
    }

    return create_void_value(vm);
  }

  /* ---- Script mode: execute foreach loop ---- */

  /* Call __iter__() */
  value_t it = value_member_call(vm, iter_val, "__iter__", 0, NULL);
  if (value_is_interrupt(it)) return it;
  if (value_is_abnormal(it)) return it;

  /* Loop: call next(), check .done, assign .value, execute body */
  for (;;) {
    value_t res = value_member_call(vm, it, "next", 0, NULL);
    if (value_is_interrupt(res)) return res;
    if (value_is_abnormal(res)) return res;

    /* Check .done */
    value_t done_val = value_get_field(vm, res, "done");
    if (value_is_interrupt(done_val)) return done_val;
    if (value_is_abnormal(done_val)) return done_val;

    value_t done_bool = value_safe_cast(vm, done_val, bool_type);
    if (value_is_interrupt(done_bool)) return done_bool;
    if (value_is_abnormal(done_bool)) return done_bool;

    bool is_done = *(bool *)value_get_data(done_bool);
    if (is_done) break;

    /* Get .value */
    value_t val_field = value_get_field(vm, res, "value");
    if (value_is_interrupt(val_field)) return val_field;
    if (value_is_abnormal(val_field)) return val_field;

    /* Assign to loop variable */
    _bind_or_update_name(vm, val_field, var_name, stmt->is_var_decl);

    /* Execute body */
    value_t body_val = run_statement(vm, stmt->body, shadow);
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      if (kind == INTERRUPT_KIND_BREAK) break;
      if (kind == INTERRUPT_KIND_CONTINUE) continue;
      return body_val;
    }
    if (value_is_abnormal(body_val)) return body_val;
  }

  return create_void_value(vm);
}
