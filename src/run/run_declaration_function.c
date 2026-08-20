#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "engine/callable_type.h"
#include "engine/ast_func.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"

/* ---- helper: extract name string from identifier node ---- */

static const char *_get_name(node_t identifier) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)identifier;
  return string_get(id->value);
}

/* ---- helper: build engine-level generic_param_t vec from AST ---- */

static vec_t _build_generic_params(vm_t vm, vec_t ast_params,
                                   scope_t scope_before, bool shadow) {
  allocator_t allocator = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
  size_t pc = vec_get_size(ast_params);

  for (size_t i = 0; i < pc; i++) {
    cubec_generic_param_t ast_param =
        (cubec_generic_param_t)vec_get(ast_params, i);
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
                               ast_param->super.location,
                               "generic param type evaluation error");
          allocator_free(allocator, &params_vec);
          return NULL;
        }
        allocator_free(allocator, &params_vec);
        return NULL;
      }
      if (type_get_kind(value_get_type(vt)) != TYPE_KIND_TYPE) {
        allocator_free(allocator, &params_vec);
        return NULL;
      }
      param_type = (type_t)value_get_data(vt);
    }

    /* evaluate extends constraints (borrowed type pointers) */
    vec_init_t evi = {.auto_dispose = false};
    vec_t extends = (vec_t)allocator_create(allocator, &g_vec_class, &evi);
    if (ast_param->constraints) {
      size_t cc = vec_get_size(ast_param->constraints);
      for (size_t j = 0; j < cc; j++) {
        node_t constraint_node = (node_t)vec_get(ast_param->constraints, j);
        value_t cv = run_expression(vm, constraint_node, false);
        if (value_is_abnormal(cv)) {
          allocator_free(allocator, &extends);
          allocator_free(allocator, &params_vec);
          return NULL;
        }
        if (type_get_kind(value_get_type(cv)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &extends);
          allocator_free(allocator, &params_vec);
          return NULL;
        }
        type_t constraint_type = (type_t)value_get_data(cv);
        vec_push(extends, constraint_type); /* borrowed: types managed by vm->types */
      }
    }

    generic_param_t gp = generic_param_create(allocator, pname, param_type,
                                                extends);
    allocator_free(allocator, &extends);
    vec_push(params_vec, gp);
  }

  return params_vec;
}

/* ---- helper: evaluate function argument types ---- */

static vec_t _eval_param_types(vm_t vm, vec_t arguments,
                               scope_t scope_before, bool shadow) {
  allocator_t allocator = vm_get_allocator(vm);
  vec_init_t ptvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(allocator, &g_vec_class, &ptvi);
  size_t arg_count = arguments ? vec_get_size(arguments) : 0;

  for (size_t i = 0; i < arg_count; i++) {
    cubec_function_argument_t param =
        (cubec_function_argument_t)vec_get(arguments, i);
    if (!param->type) {
      allocator_free(allocator, &param_types);
      return NULL;
    }
    value_t type_val = run_expression(vm, param->type, false);
    if (value_is_abnormal(type_val)) {
      allocator_free(allocator, &param_types);
      return NULL;
    }
    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
      allocator_free(allocator, &param_types);
      return NULL;
    }
    type_t pt = (type_t)value_get_data(type_val);
    vec_push(param_types, pt); /* borrowed: types managed by vm->types */
  }

  return param_types;
}

/* ---- main: construct function value from declaration node ---- */

value_t run_declaration_function(vm_t vm, node_t node, bool shadow) {
  cubec_declaration_function_t decl = (cubec_declaration_function_t)node;
  const char *name = decl->name ? _get_name(decl->name) : NULL;
  scope_t scope_before = vm_get_current_scope(vm);
  allocator_t allocator = vm_get_allocator(vm);

  /* ---- generic function ---- */
  if (decl->generic_params && vec_get_size(decl->generic_params) > 0) {
    vec_t params_vec = _build_generic_params(vm, decl->generic_params,
                                             scope_before, shadow);
    if (!params_vec) {
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "generic function param evaluation error");
        return create_void_value(vm);
      }
      return create_exception_value(vm,
          "run: generic function '%s' param evaluation error",
          name ? name : "<anonymous>");
    }

    /* create generic fn type — node is the declaration_function_t (borrowed) */
    generic_fn_type_t gt = generic_fn_type_create(allocator, name,
                                                   params_vec, decl);
    allocator_free(allocator, &params_vec);

    /* register in vm->types */
    vec_push(vm_get_types(vm), gt);

    /* create generic value: value.data = create_fn_instance callback.
     * vm_create_value_ref registers in scope->values but does NOT bind name
     * (name=NULL) — caller (run_statement_function) binds the name. */
    return vm_create_value_ref(vm, (type_t)gt,
                               (const void *)create_fn_instance, NULL);
  }

  /* ---- non-generic function ---- */

  /* evaluate parameter types */
  vec_t param_types = _eval_param_types(vm, decl->arguments,
                                         scope_before, shadow);
  if (!param_types) {
    if (shadow) {
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "function param type evaluation error");
      return create_void_value(vm);
    }
    return create_exception_value(vm,
        "run: function '%s' param type evaluation error",
        name ? name : "<anonymous>");
  }

  /* evaluate return type */
  type_t return_type;
  if (decl->return_type) {
    value_t rt_val = run_expression(vm, decl->return_type, false);
    if (value_is_abnormal(rt_val)) {
      allocator_free(allocator, &param_types);
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "function return type evaluation error");
        return create_void_value(vm);
      }
      return rt_val;
    }
    if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
      allocator_free(allocator, &param_types);
      return create_exception_value(vm,
          "run: function '%s' return type expression must produce a type, got '%s'",
          name ? name : "<anonymous>",
          type_get_name(value_get_type(rt_val)));
    }
    return_type = (type_t)value_get_data(rt_val);
  } else {
    return_type = (type_t)value_get_data(vm_get_void_type(vm));
  }

  /* create callable_type_t */
  value_t ctv = vm_create_callable_type_value(vm, param_types, return_type,
                                               decl->is_c_variadic, true,
                                               "<module>");
  allocator_free(allocator, &param_types);
  callable_type_t ct = (callable_type_t)value_get_data(ctv);

  /* create ast_func_value:
   * - declaration node for normal/comptime functions (_ast_func_exec casts
   *   af->node to cubec_declaration_function_t to extract arguments and body)
   * - NULL for extern/builtin (calling will return exception) */
  node_t decl_node = (decl->is_extern || decl->is_builtin) ? NULL : (node_t)decl;
  return create_ast_func_value(vm, ct, name, decl_node, NULL);
}
