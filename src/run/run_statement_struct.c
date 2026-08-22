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
#include "engine/struct_type.h"
#include "cubec/generic_param.h"
#include "cubec/statement_struct.h"
#include "cubec/declaration_struct.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/statement_function.h"
#include "cubec/declaration_function.h"
#include "cubec/declaration_variable.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
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

/* ---- helper: add members to a struct type value ---- */

static value_t _add_struct_members(vm_t vm, value_t type_val, vec_t members) {
  size_t mc = vec_get_size(members);
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);

    switch (member->kind) {
    case CUBEC_NODE_STRUCT_FIELD: {
      cubec_struct_field_t field = (cubec_struct_field_t)member;
      const char *field_name = _get_name(field->name);

      value_t field_type_val = run_expression(vm, field->type, false);
      if (value_is_abnormal(field_type_val))
        return field_type_val;

      if (type_get_kind(value_get_type(field_type_val)) != TYPE_KIND_TYPE)
        return create_exception_value(vm,
            "struct field '%s' type expression must produce a type, got '%s'",
            field_name, type_get_name(value_get_type(field_type_val)));

      value_t r = vm_struct_add_field(vm, type_val, field_name,
                                       field_type_val, field->is_pub);
      if (value_is_abnormal(r))
        return r;
      break;
    }

    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf = (cubec_statement_function_t)member;
      cubec_declaration_function_t decl =
          (cubec_declaration_function_t)sf->declarator;
      const char *method_name = decl->name ? _get_name(decl->name) : NULL;
      if (!method_name)
        return create_exception_value(vm,
            "struct method declaration requires a name");

      value_t func_val = run_declaration_function(vm, (node_t)decl, false);
      if (value_is_abnormal(func_val))
        return func_val;

      value_t r = vm_struct_add_prop(vm, type_val, method_name,
                                      func_val, true, true);
      if (value_is_abnormal(r))
        return r;
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION: {
      cubec_statement_declaration_t sd = (cubec_statement_declaration_t)member;
      cubec_declaration_variable_t decl =
          (cubec_declaration_variable_t)sd->declarator;
      const char *prop_name = _get_name(decl->identifier);

      value_t prop_val = run_statement_declaration(vm, member, false);
      if (value_is_abnormal(prop_val))
        return prop_val;

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, prop_name);
      if (!found || !found->ref)
        return create_exception_value(vm,
            "struct static property '%s' not found after evaluation", prop_name);

      value_t r = vm_struct_add_prop(vm, type_val, prop_name,
                                      found->ref, false, true);
      if (value_is_abnormal(r))
        return r;
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
      cubec_statement_declaration_type_t sdt =
          (cubec_statement_declaration_type_t)member;
      const char *type_name = _get_name(sdt->name);

      value_t type_result = run_statement_declaration_type(vm, member, false);
      if (value_is_abnormal(type_result))
        return type_result;

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, type_name);
      if (!found || !found->ref)
        return create_exception_value(vm,
            "struct associated type '%s' not found after evaluation", type_name);

      value_t r = vm_struct_add_prop(vm, type_val, type_name,
                                      found->ref, false, true);
      if (value_is_abnormal(r))
        return r;
      break;
    }

    default:
      break;
    }
  }
  return create_void_value(vm);
}

/* ---- main entry ---- */

value_t run_statement_struct(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* struct type expressions evaluate identically in both modes */
  cubec_statement_struct_t stmt = (cubec_statement_struct_t)node;
  const char *name = _get_name(stmt->name);
  scope_t scope_before = vm_get_current_scope(vm);
  allocator_t allocator = vm_get_allocator(vm);

  /* ---- generic struct ---- */
  if (stmt->generic_params && vec_get_size(stmt->generic_params) > 0) {
    vec_t params_vec = _build_generic_params(vm, stmt->generic_params,
                                             scope_before, shadow);
    if (!params_vec) {
      if (shadow) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "generic struct param evaluation error");
        return create_void_value(vm);
      }
      return create_exception_value(vm,
          "generic struct '%s' param evaluation error", name);
    }

    /* create generic type — node is the declaration_struct body (borrowed).
     * We use stmt itself as the node — the create_struct_instance callback
     * casts it to cubec_declaration_struct_t to access members. Since the
     * statement_struct shares the same member layout (generic_params + members),
     * we can use stmt directly as the node. */
    generic_type_t gt = generic_type_create(allocator, name, params_vec, stmt);
    allocator_free(allocator, &params_vec);

    /* register in vm->types */
    vec_push(vm_get_types(vm), gt);

    /* create generic value: value.data = create_struct_instance callback.
     * vm_create_value_ref registers in scope->values but does NOT bind name. */
    value_t generic_val = vm_create_value_ref(vm, (type_t)gt,
                           (const void *)create_struct_instance, NULL);

    /* Register self-reference in gt->scope for recursion.
     * Generic struct instances (created by create_struct_instance) need to find
     * the generic template so recursive type references trigger re-instantiation
     * with potentially different type arguments. */
    scope_t gt_scope = generic_type_get_scope(gt);
    value_t self_ref = value_create(allocator, (type_t)gt,
                                    (void *)create_struct_instance, false);
    vec_push(gt_scope->values, self_ref);
    name_t self_name = name_create(gt_scope->allocator, self_ref);
    char *owned = cstring_clone(gt_scope->allocator, name);
    strmap_insert(gt_scope->names, owned, self_name);
    allocator_free(gt_scope->allocator, &owned);

    /* bind the name in current scope */
    _bind_name(vm, generic_val, name);

    return create_void_value(vm);
  }

  /* ---- non-generic struct ---- */

  /* create the struct type value (unsealed) */
  value_t type_val = vm_create_struct_type_value(vm, name, false,
                          vm_get_current_module_id(vm));

  /* add members (fields, methods, props) */
  value_t members_result = _add_struct_members(vm, type_val, stmt->members);
  if (value_is_abnormal(members_result))
    return members_result;

  /* seal */
  value_t seal_result = vm_struct_seal(vm, type_val);
  if (value_is_abnormal(seal_result))
    return seal_result;

  /* bind the name in current scope */
  _bind_name(vm, type_val, name);

  return create_void_value(vm);
}
