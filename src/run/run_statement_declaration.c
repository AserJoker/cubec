#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "cubec/statement_declaration.h"
#include "cubec/declaration_variable.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "core/string.h"

/* ---- helper: bind a name to an already-registered value ---- */

static void _bind_name(vm_t vm, value_t val, const char *name) {
  scope_t scope = vm_get_current_scope(vm);
  if (scope && name) {
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }
}

/* ---- helper: extract name string from identifier node ---- */

static const char *_get_name(node_t identifier) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)identifier;
  return string_get(id->value);
}

/* ---- helper: evaluate type annotation and extract type_t ---- */

static type_t _eval_type(vm_t vm, node_t type_node, scope_t scope_before,
                         bool shadow) {
  value_t type_val = run_expression(vm, type_node, false);
  if (value_is_abnormal(type_val)) {
    if (shadow) {
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           type_node->location,
                           "type evaluation error in declaration");
    }
    return NULL; /* caller checks NULL and propagates */
  }
  if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
    if (shadow) {
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           type_node->location,
                           "type annotation must produce a type, got '%s'",
                           type_get_name(value_get_type(type_val)));
    }
    return NULL;
  }
  return (type_t)value_get_data(type_val);
}

/* ---- main entry ---- */

value_t run_statement_declaration(vm_t vm, node_t node, bool shadow) {
  cubec_statement_declaration_t stmt = (cubec_statement_declaration_t)node;
  cubec_declaration_variable_t decl =
      (cubec_declaration_variable_t)stmt->declarator;
  const char *name = _get_name(decl->identifier);
  scope_t scope_before = vm_get_current_scope(vm);

  /* ---- extern / builtin: create shadow value (initialized, not TDZ) ---- */
  if (stmt->is_extern || stmt->is_builtin) {
    if (!decl->type)
      return create_exception_value(vm,
                                    "%s variable '%s' requires type annotation",
                                    stmt->is_extern ? "extern" : "builtin",
                                    name);
    type_t type = _eval_type(vm, decl->type, scope_before, shadow);
    if (!type) {
      if (shadow) return create_void_value(vm);
      return create_exception_value(vm,
                                    "failed to evaluate type for %s variable '%s'",
                                    stmt->is_extern ? "extern" : "builtin",
                                    name);
    }
    vm_create_value_shadow(vm, type, name, true);
    return create_void_value(vm);
  }

  /* ---- comptime: evaluate with shadow=true ---- */
  if (stmt->is_comptime) {
    if (!decl->expression)
      return create_exception_value(vm,
                                    "comptime variable '%s' requires initializer",
                                    name);
    value_t result = run_expression(vm, decl->expression, true);
    if (value_is_abnormal(result)) {
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "comptime variable initializer error");
        return create_void_value(vm);
      }
      return result;
    }
    /* safe_cast if type annotation present */
    if (decl->type) {
      type_t annotated = _eval_type(vm, decl->type, scope_before, shadow);
      if (!annotated) {
        if (shadow) return create_void_value(vm);
        return create_exception_value(vm,
                                      "failed to evaluate type for comptime variable '%s'",
                                      name);
      }
      value_t casted = value_safe_cast(vm, result, annotated);
      if (value_is_abnormal(casted)) {
        if (shadow) {
          while (vm_get_current_scope(vm) != scope_before)
            vm_pop_scope(vm);
          diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                               node->location,
                               "comptime variable type mismatch");
          return create_void_value(vm);
        }
        return casted;
      }
      result = casted;
    }
    /* result is already in scope->values, bind the name */
    _bind_name(vm, result, name);
    return create_void_value(vm);
  }

  /* ---- normal / using: must have initializer ---- */
  if (!decl->expression)
    return create_exception_value(vm,
                                  "variable '%s' requires initializer",
                                  name);

  /* ---- undefined initializer: create TDZ shadow ---- */
  if (decl->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED) {
    if (!decl->type)
      return create_exception_value(vm,
                                    "undefined variable '%s' requires type annotation",
                                    name);
    type_t type = _eval_type(vm, decl->type, scope_before, shadow);
    if (!type) {
      if (shadow) return create_void_value(vm);
      return create_exception_value(vm,
                                    "failed to evaluate type for variable '%s'",
                                    name);
    }
    /* TDZ: initialized=false */
    vm_create_value_shadow(vm, type, name, false);
    return create_void_value(vm);
  }

  /* ---- normal: evaluate initializer ---- */
  value_t result = run_expression(vm, decl->expression, shadow);
  if (value_is_abnormal(result)) {
    if (shadow) {
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "variable initializer error");
      return create_void_value(vm);
    }
    return result;
  }

  /* safe_cast if type annotation present */
  if (decl->type) {
    type_t annotated = _eval_type(vm, decl->type, scope_before, shadow);
    if (!annotated) {
      if (shadow) return create_void_value(vm);
      return create_exception_value(vm,
                                    "failed to evaluate type for variable '%s'",
                                    name);
    }
    value_t casted = value_safe_cast(vm, result, annotated);
    if (value_is_abnormal(casted)) {
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "variable type mismatch for '%s'", name);
        return create_void_value(vm);
      }
      return casted;
    }
    result = casted;
  }

  /* result is already in scope->values, bind the name */
  _bind_name(vm, result, name);

  /* TODO: using — register defer for __dispose__ at scope exit */

  return create_void_value(vm);
}
