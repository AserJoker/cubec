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
#include "engine/pack_type.h"
#include "engine/callable_type.h"
#include "engine/interface_type.h"
#include "cubec/generic_param.h"
#include "cubec/statement_interface.h"
#include "cubec/declaration_interface.h"
#include "cubec/interface_method.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"

/* ---- helper: extract name string from identifier node ---- */

static const char *_get_name(node_t identifier) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)identifier;
  return string_get(id->value);
}

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

    type_t param_type = ast_param->is_rest
        ? (type_t)value_get_data(vm_get_pack_type(vm))
        : type_type;
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

      if (!generic_param_is_value_type_allowed(param_type)) {
        if (shadow) {
          while (vm_get_current_scope(vm) != scope_before)
            vm_pop_scope(vm);
          diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                               ast_param->super.location,
                               "generic value param '%s' has unsupported type '%s' "
                               "(only bool/integer/float/str and enum are allowed)",
                               pname ? pname : "_", type_get_name(param_type));
          allocator_free(allocator, &params_vec);
          return NULL;
        }
        allocator_free(allocator, &params_vec);
        return NULL;
      }
    }

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
        vec_push(extends, constraint_type);
      }
    }

    generic_param_t gp = generic_param_create(allocator, pname, param_type,
                                                extends, ast_param->is_rest);
    allocator_free(allocator, &extends);
    vec_push(params_vec, gp);
  }

  return params_vec;
}

/* ---- helper: add method signatures to an interface type value ---- */

static value_t _add_interface_members(vm_t vm, value_t type_val, vec_t members) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t mc = vec_get_size(members);

  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);

    if (member->kind == CUBEC_NODE_INTERFACE_METHOD) {
      cubec_interface_method_t method = (cubec_interface_method_t)member;
      const char *method_name = _get_name(method->name);

      /* evaluate parameter types */
      vec_init_t ptvi = {.auto_dispose = false};
      vec_t param_types = (vec_t)allocator_create(allocator, &g_vec_class, &ptvi);
      vec_t args = method->arguments;
      size_t arg_count = args ? vec_get_size(args) : 0;

      for (size_t j = 0; j < arg_count; j++) {
        cubec_function_argument_t param =
            (cubec_function_argument_t)vec_get(args, j);
        if (!param->type) {
          allocator_free(allocator, &param_types);
          return create_exception_value(vm,
              "interface method '%s' parameter requires type annotation",
              method_name);
        }
        value_t type_val2 = run_expression(vm, param->type, false);
        if (value_is_abnormal(type_val2)) {
          allocator_free(allocator, &param_types);
          return type_val2;
        }
        if (type_get_kind(value_get_type(type_val2)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &param_types);
          return create_exception_value(vm,
              "interface method '%s' parameter type must produce a type, got '%s'",
              method_name, type_get_name(value_get_type(type_val2)));
        }
        type_t pt = (type_t)value_get_data(type_val2);
        vec_push(param_types, pt);
      }

      /* evaluate return type */
      type_t return_type;
      if (method->return_type) {
        value_t rt_val = run_expression(vm, method->return_type, false);
        if (value_is_abnormal(rt_val)) {
          allocator_free(allocator, &param_types);
          return rt_val;
        }
        if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &param_types);
          return create_exception_value(vm,
              "interface method '%s' return type must produce a type, got '%s'",
              method_name, type_get_name(value_get_type(rt_val)));
        }
        return_type = (type_t)value_get_data(rt_val);
      } else {
        return_type = (type_t)value_get_data(vm_get_void_type(vm));
      }

      /* create callable_type value for the method signature */
      value_t ctv = vm_create_callable_type_value(vm, param_types, return_type,
                                                    false, true, "<module>");
      allocator_free(allocator, &param_types);

      /* add method to interface */
      value_t r = vm_interface_add_method(vm, type_val, method_name, ctv);
      if (value_is_abnormal(r))
        return r;
    }
    /* other member kinds (associated types, etc.) — skip for now */
  }

  return create_void_value(vm);
}

/* ---- main entry ---- */

value_t run_statement_interface(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* interface type expressions evaluate identically in both modes */
  cubec_statement_interface_t stmt = (cubec_statement_interface_t)node;
  const char *name = _get_name(stmt->name);
  scope_t scope_before = vm_get_current_scope(vm);
  allocator_t allocator = vm_get_allocator(vm);

  /* ---- generic interface ---- */
  if (stmt->generic_params && vec_get_size(stmt->generic_params) > 0) {
    vec_t params_vec = _build_generic_params(vm, stmt->generic_params,
                                             scope_before, shadow);
    if (!params_vec) {
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "generic interface param evaluation error");
        return create_void_value(vm);
      }
      return create_exception_value(vm,
          "generic interface '%s' param evaluation error", name);
    }

    /* create generic type — node is the statement_interface_t (borrowed).
     * create_interface_instance handles both statement and declaration forms. */
    generic_type_t gt = generic_type_create(allocator, name, params_vec, stmt);
    allocator_free(allocator, &params_vec);

    /* register in vm->types */
    vec_push(vm_get_types(vm), gt);

    /* create generic value: value.data = create_interface_instance callback */
    value_t generic_val = vm_create_value_ref(vm, (type_t)gt,
                           (const void *)create_interface_instance, NULL);

    /* Register self-reference in gt->scope for recursion */
    scope_t gt_scope = generic_type_get_scope(gt);
    value_t self_ref = value_create(allocator, (type_t)gt,
                                    (void *)create_interface_instance, false);
    vec_push(gt_scope->values, self_ref);
    name_t self_name = name_create(gt_scope->allocator, self_ref);
    char *owned = cstring_clone(gt_scope->allocator, name);
    strmap_insert(gt_scope->names, owned, self_name);
    allocator_free(gt_scope->allocator, &owned);

    /* bind the name in current scope */
    _bind_name(vm, generic_val, name);

    return create_void_value(vm);
  }

  /* ---- non-generic interface ---- */

  /* create the interface type value (unsealed) */
  value_t type_val = vm_create_interface_type_value(vm, name, false,
                          vm_get_current_module_id(vm));

  /* add method signatures */
  value_t members_result = _add_interface_members(vm, type_val, stmt->members);
  if (value_is_abnormal(members_result))
    return members_result;

  /* seal */
  value_t seal_result = vm_interface_seal(vm, type_val);
  if (value_is_abnormal(seal_result))
    return seal_result;

  /* bind the name in current scope */
  _bind_name(vm, type_val, name);

  return create_void_value(vm);
}
