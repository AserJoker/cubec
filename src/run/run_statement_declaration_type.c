#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/generic_type.h"
#include "engine/generic_param.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"

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

/* ---- main entry ---- */

value_t run_statement_declaration_type(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type expressions evaluate identically in both modes */
  cubec_statement_declaration_type_t stmt =
      (cubec_statement_declaration_type_t)node;
  const char *name = _get_name(stmt->name);
  scope_t scope_before = vm_get_current_scope(vm);

  /* ---- builtin: verify against global scope, register name in current scope
   * ---- */
  if (stmt->is_builtin) {
    /* builtin + generic: create generic type with C callback from
     * builtin_templates */
    if (stmt->params && vec_get_size(stmt->params) > 0) {
      allocator_t allocator = vm_get_allocator(vm);
      create_instance_fn_t callback =
          vm_get_builtin_template(vm, name);
      if (!callback)
        return create_exception_value(vm,
            "run: builtin generic type '%s' not found in builtin_templates",
            name);

      /* build engine-level generic_param_t vec from AST */
      type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
      vec_init_t pvi = {.auto_dispose = true};
      vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
      size_t pc = vec_get_size(stmt->params);
      for (size_t i = 0; i < pc; i++) {
        cubec_generic_param_t ast_param =
            (cubec_generic_param_t)vec_get(stmt->params, i);
        const char *pname = _get_name(ast_param->name);

        /* determine param type */
        type_t param_type = type_type;
        if (ast_param->value_type) {
          value_t vt = run_expression(vm, ast_param->value_type, false);
          if (value_is_abnormal(vt)) {
            if (shadow) {
              while (vm_get_current_scope(vm) != scope_before)
                vm_pop_scope(vm);
              diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                                   node->location,
                                   "builtin generic param type evaluation error");
              allocator_free(allocator, &params_vec);
              return create_void_value(vm);
            }
            allocator_free(allocator, &params_vec);
            return vt;
          }
          if (type_get_kind(value_get_type(vt)) != TYPE_KIND_TYPE) {
            allocator_free(allocator, &params_vec);
            return create_exception_value(vm,
                "run: builtin generic param '%s' type annotation must produce a type, got '%s'",
                pname, type_get_name(value_get_type(vt)));
          }
          param_type = (type_t)value_get_data(vt);
        }

        /* evaluate extends constraints */
        vec_init_t evi = {.auto_dispose = true};
        vec_t extends = (vec_t)allocator_create(allocator, &g_vec_class, &evi);
        if (ast_param->constraints) {
          size_t cc = vec_get_size(ast_param->constraints);
          for (size_t j = 0; j < cc; j++) {
            node_t constraint_node = (node_t)vec_get(ast_param->constraints, j);
            value_t cv = run_expression(vm, constraint_node, false);
            if (value_is_abnormal(cv)) {
              allocator_free(allocator, &extends);
              allocator_free(allocator, &params_vec);
              if (shadow) {
                while (vm_get_current_scope(vm) != scope_before)
                  vm_pop_scope(vm);
                diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                                     node->location,
                                     "builtin generic param constraint evaluation error");
                return create_void_value(vm);
              }
              return cv;
            }
            if (type_get_kind(value_get_type(cv)) != TYPE_KIND_TYPE) {
              allocator_free(allocator, &extends);
              allocator_free(allocator, &params_vec);
              return create_exception_value(vm,
                  "run: builtin generic param constraint must produce a type, got '%s'",
                  type_get_name(value_get_type(cv)));
            }
            type_t constraint_type = (type_t)value_get_data(cv);
            vec_push(extends, alloc_clone(allocator, constraint_type));
          }
        }

        generic_param_t gp = generic_param_create(allocator, pname, param_type,
                                                    extends);
        allocator_free(allocator, &extends);
        vec_push(params_vec, gp);
      }

      /* create generic type — node=NULL (builtin has no AST) */
      generic_type_t gt = generic_type_create(allocator, name, params_vec, NULL);
      allocator_free(allocator, &params_vec);

      /* register in scope->types */
      scope_t scope = vm_get_current_scope(vm);
      vec_push(scope->types, gt);

      /* create generic value: value.data = builtin callback.
       * vm_create_value_ref registers in scope->values and binds the name. */
      vm_create_value_ref(vm, (type_t)gt, (const void *)callback, name);
      return create_void_value(vm);
    }

    /* builtin non-generic: look up concrete type in global scope */
    scope_t global_scope = vm_get_global_scope(vm);
    name_t builtin_name = scope_lookup(global_scope, name);
    if (!builtin_name || !builtin_name->ref)
      return create_exception_value(vm,
                                    "run: builtin type '%s' not found", name);
    value_t builtin_val = builtin_name->ref;

    /* if type_value is provided, verify it matches the builtin */
    if (stmt->type_value) {
      value_t type_val = run_expression(vm, stmt->type_value, false);
      if (value_is_abnormal(type_val)) {
        if (shadow) {
          while (vm_get_current_scope(vm) != scope_before)
            vm_pop_scope(vm);
          diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                               node->location,
                               "builtin type declaration evaluation error");
          return create_void_value(vm);
        }
        return type_val;
      }
      value_t eq = value_equal(vm, builtin_val, type_val);
      if (value_is_abnormal(eq)) {
        if (shadow) {
          while (vm_get_current_scope(vm) != scope_before)
            vm_pop_scope(vm);
          diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                               node->location,
                               "builtin type '%s' declaration comparison error",
                               name);
          return create_void_value(vm);
        }
        return eq;
      }
      bool equal = *(bool *)value_get_data(eq);
      if (!equal)
        return create_exception_value(vm,
                                      "run: builtin type '%s' declaration mismatches",
                                      name);
    }

    /* register the builtin value's name in current scope (ref, not clone) */
    _bind_name(vm, builtin_val, name);
    return create_void_value(vm);
  }

  /* ---- normal type alias ---- */

  /* ---- generic type alias ---- */
  if (stmt->params && vec_get_size(stmt->params) > 0) {
    if (!stmt->type_value)
      return create_exception_value(vm,
                                    "run: generic type alias '%s' requires type expression",
                                    name);

    /* build engine-level generic_param_t vec from AST cubec_generic_param_t
     * nodes */
    allocator_t allocator = vm_get_allocator(vm);
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    vec_init_t pvi = {.auto_dispose = true};
    vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
    size_t pc = vec_get_size(stmt->params);
    for (size_t i = 0; i < pc; i++) {
      cubec_generic_param_t ast_param =
          (cubec_generic_param_t)vec_get(stmt->params, i);
      const char *pname = _get_name(ast_param->name);

      /* determine param type: value_type annotation → eval it; otherwise type
       * param (the "type" type) */
      type_t param_type = type_type;
      if (ast_param->value_type) {
        value_t vt = run_expression(vm, ast_param->value_type, false);
        if (value_is_abnormal(vt)) {
          if (shadow) {
            while (vm_get_current_scope(vm) != scope_before)
              vm_pop_scope(vm);
            diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                                 node->location,
                                 "generic param type evaluation error");
            allocator_free(allocator, &params_vec);
            return create_void_value(vm);
          }
          allocator_free(allocator, &params_vec);
          return vt;
        }
        if (type_get_kind(value_get_type(vt)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &params_vec);
          return create_exception_value(vm,
              "run: generic param '%s' type annotation must produce a type, got '%s'",
              pname, type_get_name(value_get_type(vt)));
        }
        param_type = (type_t)value_get_data(vt);
      }

      /* evaluate extends constraints */
      vec_init_t evi = {.auto_dispose = true};
      vec_t extends = (vec_t)allocator_create(allocator, &g_vec_class, &evi);
      if (ast_param->constraints) {
        size_t cc = vec_get_size(ast_param->constraints);
        for (size_t j = 0; j < cc; j++) {
          node_t constraint_node = (node_t)vec_get(ast_param->constraints, j);
          value_t cv = run_expression(vm, constraint_node, false);
          if (value_is_abnormal(cv)) {
            allocator_free(allocator, &extends);
            allocator_free(allocator, &params_vec);
            if (shadow) {
              while (vm_get_current_scope(vm) != scope_before)
                vm_pop_scope(vm);
              diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                                   node->location,
                                   "generic param constraint evaluation error");
              return create_void_value(vm);
            }
            return cv;
          }
          if (type_get_kind(value_get_type(cv)) != TYPE_KIND_TYPE) {
            allocator_free(allocator, &extends);
            allocator_free(allocator, &params_vec);
            return create_exception_value(vm,
                "run: generic param constraint must produce a type, got '%s'",
                type_get_name(value_get_type(cv)));
          }
          type_t constraint_type = (type_t)value_get_data(cv);
          vec_push(extends, alloc_clone(allocator, constraint_type));
        }
      }

      generic_param_t gp = generic_param_create(allocator, pname, param_type,
                                                  extends);
      allocator_free(allocator, &extends);
      vec_push(params_vec, gp);
    }

    /* create generic type — node is the RHS type expression (borrowed) */
    generic_type_t gt = generic_type_create(allocator, name, params_vec,
                                            stmt->type_value);
    allocator_free(allocator, &params_vec);

    /* register in scope->types */
    scope_t scope = vm_get_current_scope(vm);
    vec_push(scope->types, gt);

    /* create generic value: value.data = create_type_instance callback.
     * vm_create_value_ref registers in scope->values and binds the name. */
    vm_create_value_ref(vm, (type_t)gt, (const void *)create_type_instance, name);
    return create_void_value(vm);
  }

  if (!stmt->type_value)
    return create_exception_value(vm,
                                  "run: type alias '%s' requires type expression",
                                  name);

  value_t type_val = run_expression(vm, stmt->type_value, false);
  if (value_is_abnormal(type_val)) {
    if (shadow) {
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "type alias evaluation error");
      return create_void_value(vm);
    }
    return type_val;
  }

  if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm,
                                  "run: type alias expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(type_val)));

  /* type_val is already in scope->values; just bind the name */
  _bind_name(vm, type_val, name);
  return create_void_value(vm);
}
