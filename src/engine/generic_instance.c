#include "core/string.h"
#include "core/vec.h"
#include "cubec/declaration_function.h"
#include "cubec/declaration_struct.h"
#include "cubec/declaration_union.h"
#include "cubec/declaration_interface.h"
#include "cubec/declaration_variable.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/interface_method.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "engine/ast_func.h"
#include "engine/callable_type.h"
#include "engine/diagnostic.h"
#include "engine/exception_type.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "engine/generic_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/interface_type.h"
#include "engine/tuple_type.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "run/run.h"
#include <stdbool.h>
#include <string.h>

/* ---- Error-return helper for temp scope ---- */

/**
 * @brief Pop the current (temporary) scope and return a clone of the error
 * value in the restored parent scope. Prevents returning a dangling pointer
 * when the error was registered in the scope being popped (auto_dispose frees
 * all values in scope->values, including the exception/interrupt).
 *
 * The caller should do: vec_remove(cache, idx) → _pop_scope_return_error.
 */
static value_t _pop_scope_return_error(vm_t vm, value_t err) {
  /* Clone err into the parent scope BEFORE popping, so the clone survives
   * the scope disposal. Switch to parent scope temporarily so value_clone
   * registers the clone there. */
  scope_t temp = vm_get_current_scope(vm);
  scope_t parent = temp->parent;
  vm_set_scope(vm, parent);
  value_t cloned = value_clone(vm, err);
  vm_set_scope(vm, temp);
  vm_pop_scope(vm);
  return cloned;
}

/* ---- Shared cache lookup ---- */

/**
 * @brief Search the instance cache for a matching set of arguments.
 * Uses value_equal for semantic comparison of each argument.
 * @return the cached instance value if found, NULL otherwise (miss).
 *         May return an exception value if value_equal fails.
 */
static value_t _cache_lookup(vm_t vm, generic_type_t gt, size_t argc,
                             value_t *argv) {
  vec_t instances = generic_type_get_instances(gt);
  size_t n = vec_get_size(instances);
  for (size_t i = 0; i < n; i++) {
    generic_instance_t gi = (generic_instance_t)vec_get(instances, i);
    vec_t cached_params = generic_instance_get_params(gi);
    size_t pc = vec_get_size(cached_params);
    if (pc != argc)
      continue;
    bool match = true;
    for (size_t j = 0; j < pc && match; j++) {
      value_t cached = (value_t)vec_get(cached_params, j);
      value_t eq = value_equal(vm, argv[j], cached);
      /* On comparison error (e.g. type-kind mismatch when comparing type
       * values), this cached entry cannot match — skip it and try the next.
       * The error value is registered on the current scope and will be
       * cleaned up on scope_dispose; it must not propagate, since "cannot
       * compare" for cache purposes means "not equal", not a real failure. */
      if (value_is_abnormal(eq)) {
        match = false;
        break;
      }
      bool ok = *(bool *)value_get_data(eq);
      if (!ok)
        match = false;
    }
    if (match)
      return generic_instance_get_instance(gi);
  }
  return NULL;
}

/* ---- Shared parameter binding ---- */

/**
 * @brief Push a temporary scope as child of the current scope and bind
 * generic parameter names to concrete argument values.
 *
 * For pack params (is_rest=true), all remaining argv values are collected
 * into a tuple type value bound to the pack param name.
 *
 * Argument values are borrowed (owned by the caller's scope); we only bind
 * names, and do NOT push argv into temp->values — that would double-free on
 * scope_dispose.
 *
 * @return the temporary scope (caller must vm_pop_scope after use;
 *         vm_pop_scope disposes the scope automatically)
 */
static scope_t _bind_params(vm_t vm, generic_type_t gt, size_t argc,
                            value_t *argv) {
  allocator_t allocator = vm_get_allocator(vm);
  scope_t parent = vm_get_current_scope(vm);
  scope_t temp = scope_create(allocator, SCOPE_TYPE, parent, NULL);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  vec_t param_defs = generic_type_get_params(gt);
  size_t param_count = vec_get_size(param_defs);
  size_t argv_idx = 0;

  for (size_t i = 0; i < param_count; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    const char *pname = generic_param_get_name(gp);

    if (generic_param_is_rest(gp)) {
      /* Pack param: collect remaining argv into a tuple type value.
       * Always process pack params even when argv_idx >= argc (empty pack),
       * so the name is bound to an empty tuple type value. */
      vec_init_t etvi = {.auto_dispose = false};
      vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_class, &etvi);
      while (argv_idx < argc) {
        /* argv[argv_idx] is a type value (TYPE_KIND_TYPE) — extract the type */
        if (type_get_kind(value_get_type(argv[argv_idx])) == TYPE_KIND_TYPE) {
          type_t t = (type_t)value_get_data(argv[argv_idx]);
          vec_push(element_types, t);
        }
        argv_idx++;
      }
      /* create a tuple type wrapping all absorbed types (may be empty) */
      tuple_type_t tt = tuple_type_create(allocator, element_types, true);
      allocator_free(allocator, &element_types);
      vec_push(vm_get_types(vm), tt); /* register so vm_dispose frees it */
      /* wrap as type value and bind */
      value_t pack_val = vm_create_value_ref(vm, type_type, (type_t)tt, pname);
      name_t name = name_create(temp->allocator, pack_val);
      char *owned = cstring_clone(temp->allocator, pname);
      strmap_insert(temp->names, owned, name);
      allocator_free(temp->allocator, &owned);
    } else {
      if (argv_idx >= argc)
        break; /* not enough args for remaining non-rest params */
      /* Normal param: bind name -> single concrete value */
      name_t name = name_create(temp->allocator, argv[argv_idx]);
      strmap_insert(temp->names, pname, name);
      argv_idx++;
    }
  }

  vm_push_scope(vm, temp);
  return temp;
}

/* ---- create_struct_instance ---- */

value_t create_struct_instance(vm_t vm, value_t tmpl, size_t argc,
                               value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_type_t gt = (generic_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup — NULL means miss, non-NULL means hit or error */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. get AST node — may be declaration_struct_t (expression form)
   *    or statement_struct_t (statement form). Both have .members. */
  void *raw_node = generic_type_get_node(gt);
  if (!raw_node)
    return create_exception_value(vm, "generic struct '%s' is missing its definition",
                                  type_get_name(self_type));
  node_t node = (node_t)raw_node;
  vec_t members;
  if (node->kind == CUBEC_NODE_STATEMENT_STRUCT) {
    cubec_statement_struct_t stmt = (cubec_statement_struct_t)node;
    members = stmt->members;
  } else {
    cubec_declaration_struct_t decl = (cubec_declaration_struct_t)node;
    members = decl->members;
  }

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. create the concrete struct type (registers on temp scope) */
  const char *base_name = type_get_name(self_type);
  value_t type_val =
      vm_create_struct_type_value(vm, base_name, false, "<builtin>");

  /* 5. early-cache: clone the unsealed type value into gt's isolated scope
   *    and register it in the instance cache BEFORE processing members.
   *    This allows self-referential types (e.g. `f: *Result[T, E]`) to
   *    resolve via cache during field type evaluation. Both the clone and
   *    the original share the same struct_type_t — seal/field additions
   *    are visible through both value_t handles. */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_type_get_scope(gt);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, type_val);
  vm_set_scope(vm, orig_scope);

  if (value_is_abnormal(instance)) {
    vm_pop_scope(vm);
    return instance;
  }

  /* build cache entry pointing to the early-cached instance */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);
  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_t instances = generic_type_get_instances(gt);
  size_t cache_idx = vec_get_size(instances);
  vec_push(instances, gi);

  /* 6. first pass: process fields only — type layout must be complete
   *    before methods/props can reference the sealed type. */
  size_t mc = vec_get_size(members);
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);
    if (member->kind != CUBEC_NODE_STRUCT_FIELD)
      continue;

    cubec_struct_field_t field = (cubec_struct_field_t)member;
    const char *field_name =
        string_get(((cubec_literal_identifier_t)field->name)->value);

    /* evaluate the field's type expression with parameter substitution */
    value_t field_type_val = run_expression(vm, field->type, false);

    if (value_is_abnormal(field_type_val)) {
      vec_remove(instances, cache_idx);
      return _pop_scope_return_error(vm, field_type_val);
    }

    /* field type expression must produce a type value */
    type_kind_t ftk = type_get_kind(value_get_type(field_type_val));
    if (ftk != TYPE_KIND_TYPE) {
      value_t err = create_exception_value(
          vm, "struct field '%s' type expression must produce a type, got '%s'",
          field_name, type_get_name(value_get_type(field_type_val)));
      vec_remove(instances, cache_idx);
      return _pop_scope_return_error(vm, err);
    }

    value_t add_result = vm_struct_add_field(vm, type_val, field_name, field_type_val,
                        field->is_pub);
    if (value_is_abnormal(add_result)) {
      vec_remove(instances, cache_idx);
      return _pop_scope_return_error(vm, add_result);
    }
  }

  /* 7. seal — computes final size/alignment; fields can no longer be added */
  vm_struct_seal(vm, type_val);

  /* 8. second pass: process methods, props, and associated types.
   *    These can safely reference the now-sealed type (e.g. self: *T). */
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);

    switch (member->kind) {
    case CUBEC_NODE_STRUCT_FIELD:
      break; /* already processed in first pass */

    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf = (cubec_statement_function_t)member;
      cubec_declaration_function_t decl =
          (cubec_declaration_function_t)sf->declarator;
      const char *method_name = decl->name
          ? string_get(((cubec_literal_identifier_t)decl->name)->value)
          : NULL;
      if (!method_name) {
        vec_remove(instances, cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "struct method declaration requires a name");
      }

      value_t func_val = run_declaration_function(vm, (node_t)decl, false);
      if (value_is_abnormal(func_val)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, func_val);
      }

      value_t r = vm_struct_add_prop(vm, type_val, method_name,
                                      func_val, true, true);
      if (value_is_abnormal(r)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION: {
      cubec_statement_declaration_t sd = (cubec_statement_declaration_t)member;
      cubec_declaration_variable_t decl =
          (cubec_declaration_variable_t)sd->declarator;
      const char *prop_name =
          string_get(((cubec_literal_identifier_t)decl->identifier)->value);

      value_t prop_val = run_statement_declaration(vm, member, false);
      if (value_is_abnormal(prop_val)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, prop_val);
      }

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, prop_name);
      if (!found || !found->ref) {
        vec_remove(instances, cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "struct static property '%s' not found after evaluation", prop_name);
      }

      value_t r = vm_struct_add_prop(vm, type_val, prop_name,
                                      found->ref, false, true);
      if (value_is_abnormal(r)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
      cubec_statement_declaration_type_t sdt =
          (cubec_statement_declaration_type_t)member;
      const char *type_name =
          string_get(((cubec_literal_identifier_t)sdt->name)->value);

      value_t type_result = run_statement_declaration_type(vm, member, false);
      if (value_is_abnormal(type_result)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, type_result);
      }

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, type_name);
      if (!found || !found->ref) {
        vec_remove(instances, cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "struct associated type '%s' not found after evaluation", type_name);
      }

      value_t r = vm_struct_add_prop(vm, type_val, type_name,
                                      found->ref, false, true);
      if (value_is_abnormal(r)) {
        vec_remove(instances, cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    default:
      break;
    }
  }

  /* 9. pop+dispose temp scope (restores parent, frees temp's registrations).
   *    The instance in gt->scope survives because it was cloned there in step 5. */
  vm_pop_scope(vm);

  return instance;
}

/* ---- create_union_instance ---- */

value_t create_union_instance(vm_t vm, value_t tmpl, size_t argc,
                              value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_type_t gt = (generic_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. get AST node — may be declaration_union_t (expression form)
   *    or statement_union_t (statement form). */
  void *raw_node = generic_type_get_node(gt);
  if (!raw_node)
    return create_exception_value(vm, "generic union '%s' is missing its definition",
                                  type_get_name(self_type));
  node_t node = (node_t)raw_node;
  vec_t members;
  if (node->kind == CUBEC_NODE_STATEMENT_UNION) {
    cubec_statement_union_t stmt = (cubec_statement_union_t)node;
    members = stmt->members;
  } else {
    cubec_declaration_union_t decl = (cubec_declaration_union_t)node;
    members = decl->members;
  }

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. create the concrete union type (registers on temp scope) */
  const char *base_name = type_get_name(self_type);
  value_t type_val =
      vm_create_union_type_value(vm, base_name, false, "<builtin>");

  /* 5. early-cache: clone the unsealed type value into gt's isolated scope
   *    and register in the instance cache BEFORE processing members.
   *    This allows self-referential types to resolve via cache. */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_type_get_scope(gt);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, type_val);
  vm_set_scope(vm, orig_scope);

  if (value_is_abnormal(instance)) {
    vm_pop_scope(vm);
    return instance;
  }

  /* build cache entry */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);
  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_t u_instances = generic_type_get_instances(gt);
  size_t u_cache_idx = vec_get_size(u_instances);
  vec_push(u_instances, gi);

  /* 6. first pass: process fields only */
  size_t mc = vec_get_size(members);
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);

    if (member->kind == CUBEC_NODE_UNION_FIELD) {
      cubec_union_field_t field = (cubec_union_field_t)member;
      const char *field_name =
          string_get(((cubec_literal_identifier_t)field->name)->value);

      value_t field_type_val = run_expression(vm, field->type, false);
      if (value_is_abnormal(field_type_val)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, field_type_val);
      }

      if (type_get_kind(value_get_type(field_type_val)) != TYPE_KIND_TYPE) {
        value_t err = create_exception_value(
            vm, "union field '%s' type expression must produce a type, got '%s'",
            field_name, type_get_name(value_get_type(field_type_val)));
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, err);
      }

      value_t add_result = vm_union_add_field(vm, type_val, field_name, field_type_val, true);
      if (value_is_abnormal(add_result)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, add_result);
      }
    } else if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
      /* struct_field is used in some AST contexts (e.g. cunion) */
      cubec_struct_field_t field = (cubec_struct_field_t)member;
      const char *field_name =
          string_get(((cubec_literal_identifier_t)field->name)->value);

      value_t field_type_val = run_expression(vm, field->type, false);
      if (value_is_abnormal(field_type_val)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, field_type_val);
      }

      if (type_get_kind(value_get_type(field_type_val)) != TYPE_KIND_TYPE) {
        value_t err = create_exception_value(
            vm, "union field '%s' type expression must produce a type, got '%s'",
            field_name, type_get_name(value_get_type(field_type_val)));
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, err);
      }

      value_t add_result2 = vm_union_add_field(vm, type_val, field_name, field_type_val,
                         field->is_pub);
      if (value_is_abnormal(add_result2)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, add_result2);
      }
    }
  }

  /* 7. seal */
  vm_union_seal(vm, type_val);

  /* 8. second pass: process methods, props, and associated types */
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);

    switch (member->kind) {
    case CUBEC_NODE_UNION_FIELD:
    case CUBEC_NODE_STRUCT_FIELD:
      break; /* already processed in first pass */

    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf = (cubec_statement_function_t)member;
      cubec_declaration_function_t decl =
          (cubec_declaration_function_t)sf->declarator;
      const char *method_name = decl->name
          ? string_get(((cubec_literal_identifier_t)decl->name)->value)
          : NULL;
      if (!method_name) {
        vec_remove(u_instances, u_cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "union method declaration requires a name");
      }

      value_t func_val = run_declaration_function(vm, (node_t)decl, false);
      if (value_is_abnormal(func_val)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, func_val);
      }

      value_t r = vm_union_add_prop(vm, type_val, method_name,
                                     func_val, true, true);
      if (value_is_abnormal(r)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION: {
      cubec_statement_declaration_t sd = (cubec_statement_declaration_t)member;
      cubec_declaration_variable_t decl =
          (cubec_declaration_variable_t)sd->declarator;
      const char *prop_name =
          string_get(((cubec_literal_identifier_t)decl->identifier)->value);

      value_t prop_val = run_statement_declaration(vm, member, false);
      if (value_is_abnormal(prop_val)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, prop_val);
      }

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, prop_name);
      if (!found || !found->ref) {
        vec_remove(u_instances, u_cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "union static property '%s' not found after evaluation", prop_name);
      }

      value_t r = vm_union_add_prop(vm, type_val, prop_name,
                                     found->ref, false, true);
      if (value_is_abnormal(r)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
      cubec_statement_declaration_type_t sdt =
          (cubec_statement_declaration_type_t)member;
      const char *type_name =
          string_get(((cubec_literal_identifier_t)sdt->name)->value);

      value_t type_result = run_statement_declaration_type(vm, member, false);
      if (value_is_abnormal(type_result)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, type_result);
      }

      scope_t scope = vm_get_current_scope(vm);
      name_t found = scope_lookup(scope, type_name);
      if (!found || !found->ref) {
        vec_remove(u_instances, u_cache_idx);
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "union associated type '%s' not found after evaluation", type_name);
      }

      value_t r = vm_union_add_prop(vm, type_val, type_name,
                                     found->ref, false, true);
      if (value_is_abnormal(r)) {
        vec_remove(u_instances, u_cache_idx);
        return _pop_scope_return_error(vm, r);
      }
      break;
    }

    default:
      break;
    }
  }

  /* 9. pop+dispose temp scope.
   *    The instance in gt->scope survives because it was cloned there in step 5. */
  vm_pop_scope(vm);

  return instance;
}

/* ---- create_interface_instance ---- */

value_t create_interface_instance(vm_t vm, value_t tmpl, size_t argc,
                                  value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_type_t gt = (generic_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. get AST node — may be declaration_interface_t (expression form)
   *    or statement_interface_t (statement form). */
  void *raw_node = generic_type_get_node(gt);
  if (!raw_node)
    return create_exception_value(vm, "generic interface '%s' is missing its definition",
                                  type_get_name(self_type));
  node_t node = (node_t)raw_node;
  vec_t members;
  if (node->kind == CUBEC_NODE_STATEMENT_INTERFACE) {
    cubec_statement_interface_t stmt = (cubec_statement_interface_t)node;
    members = stmt->members;
  } else {
    cubec_declaration_interface_t decl = (cubec_declaration_interface_t)node;
    members = decl->members;
  }

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. create the concrete interface type (registers on temp scope) */
  const char *base_name = type_get_name(self_type);
  value_t type_val =
      vm_create_interface_type_value(vm, base_name, false, "<builtin>");

  /* 5. iterate members, add method signatures */
  size_t mc = vec_get_size(members);
  for (size_t i = 0; i < mc; i++) {
    node_t member = (node_t)vec_get(members, i);
    if (member->kind != CUBEC_NODE_INTERFACE_METHOD)
      continue;

    cubec_interface_method_t method = (cubec_interface_method_t)member;
    const char *method_name =
        string_get(((cubec_literal_identifier_t)method->name)->value);

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
        vm_pop_scope(vm);
        return create_exception_value(vm,
            "interface method '%s' parameter requires type annotation",
            method_name);
      }
      value_t pt_val = run_expression(vm, param->type, false);
      if (value_is_abnormal(pt_val)) {
        allocator_free(allocator, &param_types);
        return _pop_scope_return_error(vm, pt_val);
      }
      if (type_get_kind(value_get_type(pt_val)) != TYPE_KIND_TYPE) {
        value_t err = create_exception_value(vm,
            "interface method '%s' parameter type must produce a type, got '%s'",
            method_name, type_get_name(value_get_type(pt_val)));
        allocator_free(allocator, &param_types);
        return _pop_scope_return_error(vm, err);
      }
      type_t pt = (type_t)value_get_data(pt_val);
      vec_push(param_types, pt);
    }

    /* evaluate return type */
    type_t return_type;
    if (method->return_type) {
      value_t rt_val = run_expression(vm, method->return_type, false);
      if (value_is_abnormal(rt_val)) {
        allocator_free(allocator, &param_types);
        return _pop_scope_return_error(vm, rt_val);
      }
      if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
        value_t err = create_exception_value(vm,
            "interface method '%s' return type must produce a type, got '%s'",
            method_name, type_get_name(value_get_type(rt_val)));
        allocator_free(allocator, &param_types);
        return _pop_scope_return_error(vm, err);
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
    if (value_is_abnormal(r)) {
      return _pop_scope_return_error(vm, r);
    }
  }

  /* 6. seal */
  value_t seal_r = vm_interface_seal(vm, type_val);
  if (value_is_abnormal(seal_r)) {
    return _pop_scope_return_error(vm, seal_r);
  }

  /* 7. clone the sealed interface type value into the generic's isolated scope
   *    BEFORE popping temp scope. */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_type_get_scope(gt);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, type_val);
  vm_set_scope(vm, orig_scope);

  /* 8. pop+dispose temp scope */
  vm_pop_scope(vm);

  if (value_is_abnormal(instance))
    return instance;

  /* 9. build cache entry */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);

  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_push(generic_type_get_instances(gt), gi);

  return instance;
}

/* ---- create_type_instance ---- */

value_t create_type_instance(vm_t vm, value_t tmpl, size_t argc,
                             value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_type_t gt = (generic_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup — NULL means miss, non-NULL means hit or error */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. get AST node — for a type alias, the node stored in generic_type_t
   *    is the RHS type_value expression to evaluate. */
  void *node = generic_type_get_node(gt);
  if (!node)
    return create_exception_value(vm, "generic type '%s' is missing its definition",
                                  type_get_name(self_type));
  node_t type_expr = (node_t)node;

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. evaluate the type expression with parameter substitution */
  value_t result = run_expression(vm, type_expr, false);

  if (value_is_abnormal(result)) {
    return _pop_scope_return_error(vm, result);
  }

  /* result must be a type value */
  if (type_get_kind(value_get_type(result)) != TYPE_KIND_TYPE) {
    value_t err = create_exception_value(
        vm, "type alias expression must produce a type, got '%s'",
        type_get_name(value_get_type(result)));
    return _pop_scope_return_error(vm, err);
  }

  /* 5. clone the result type value into the generic's isolated scope BEFORE
   *    popping temp scope. vm_pop_scope disposes temp, which frees result —
   *    so we must clone while result is still alive. */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_type_get_scope(gt);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, result);
  vm_set_scope(vm, orig_scope);

  /* 6. pop+dispose temp scope (restores parent, frees temp's registrations) */
  vm_pop_scope(vm);

  if (value_is_abnormal(instance))
    return instance;

  /* 8. build cache entry (borrows both params vec + instance).
   *     params_vec holds borrowed argv pointers (owned by caller's scope);
   *     instance is borrowed from gt_scope->values. auto_dispose=false so
   *     freeing the cache entry releases only the vec structure, not the
   *     borrowed values. */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);

  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_push(generic_type_get_instances(gt), gi);

  return instance;
}

/* ---- create_remove_const_instance ---- */

value_t create_remove_const_instance(vm_t vm, value_t tmpl, size_t argc,
                                     value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_type_t gt = (generic_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. argv[0] must be a type value wrapping a const type */
  value_t arg = argv[0];
  if (type_get_kind(value_get_type(arg)) != TYPE_KIND_TYPE)
    return create_exception_value(
        vm, "remove_const: argument must be a type, got '%s'",
        type_get_name(value_get_type(arg)));

  type_t const_type = (type_t)value_get_data(arg);
  if (type_is_mut(const_type))
    return create_exception_value(vm,
                                  "remove_const: type '%s' is already mutable",
                                  type_get_name(const_type));

  /* 3. create a mutable variant of the type */
  type_t mut_type = type_create_with_mut(vm, const_type, true);

  /* 4. create a type value wrapping the mutable type in the generic's scope */
  scope_t gt_scope = generic_type_get_scope(gt);
  scope_t orig_scope = vm_get_current_scope(vm);
  vm_set_scope(vm, gt_scope);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t instance = vm_create_value_ref(vm, type_type, mut_type, NULL);
  vm_set_scope(vm, orig_scope);

  /* 5. build cache entry */
  vec_init_t vi2 = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi2);
  vec_push(params_vec, argv[0]);

  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_push(generic_type_get_instances(gt), gi);

  return instance;
}

/* ---- create_fn_instance ---- */

value_t create_fn_instance(vm_t vm, value_t tmpl, size_t argc, value_t *argv) {
  type_t self_type = value_get_type(tmpl);
  generic_fn_type_t gt_fn = (generic_fn_type_t)self_type;
  generic_type_t gt = (generic_type_t)gt_fn; /* layout-compatible cast */
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. cache lookup — NULL means miss, non-NULL means hit or error */
  value_t cached = _cache_lookup(vm, gt, argc, argv);
  if (cached)
    return cached;

  /* 2. get AST node */
  cubec_declaration_function_t decl =
      (cubec_declaration_function_t)generic_fn_type_get_node(gt_fn);
  if (!decl)
    return create_exception_value(vm, "generic function '%s' is missing its definition",
                                  type_get_name(self_type));

  /* 3. bind generic params in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. evaluate parameter type expressions (generic params are now in scope) */
  vec_init_t ptvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(allocator, &g_vec_class, &ptvi);
  vec_t args = decl->arguments;
  size_t arg_count = args ? vec_get_size(args) : 0;
  for (size_t i = 0; i < arg_count; i++) {
    cubec_function_argument_t param =
        (cubec_function_argument_t)vec_get(args, i);
    if (!param->type) {
      vm_pop_scope(vm);
      allocator_free(allocator, &param_types);
      const char *pname =
          param->identifier
              ? string_get(
                    ((cubec_literal_identifier_t)param->identifier)->value)
              : "<anonymous>";
      return create_exception_value(
          vm, "generic function parameter '%s' requires type annotation",
          pname);
    }
    value_t type_val = run_expression(vm, param->type, false);
    if (value_is_abnormal(type_val)) {
      allocator_free(allocator, &param_types);
      return _pop_scope_return_error(vm, type_val);
    }
    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
      value_t err = create_exception_value(vm,
                                    "generic function parameter type "
                                    "expression must produce a type, got '%s'",
                                    type_get_name(value_get_type(type_val)));
      allocator_free(allocator, &param_types);
      return _pop_scope_return_error(vm, err);
    }
    type_t pt = (type_t)value_get_data(type_val);
    /* For rest params (...args: T), the type expression evaluates to a tuple
     * type (because the pack param T is bound to a tuple type value).
     * Expand the tuple's element types as individual parameters in the callable
     * signature so that value_call sees the correct argc. */
    if (param->is_rest && type_get_kind(pt) == TYPE_KIND_TUPLE) {
      tuple_type_t tt = (tuple_type_t)pt;
      uint64_t elem_count = tuple_type_get_field_count(tt);
      for (uint64_t e = 0; e < elem_count; e++)
        vec_push(param_types, tuple_type_get_element_type(tt, e));
    } else {
      vec_push(param_types, pt); /* borrowed: types managed by vm->types */
    }
  }

  /* 5. evaluate return type expression */
  type_t return_type;
  if (decl->return_type) {
    value_t rt_val = run_expression(vm, decl->return_type, false);
    if (value_is_abnormal(rt_val)) {
      allocator_free(allocator, &param_types);
      return _pop_scope_return_error(vm, rt_val);
    }
    if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
      value_t err = create_exception_value(vm,
                                    "generic function return type expression "
                                    "must produce a type, got '%s'",
                                    type_get_name(value_get_type(rt_val)));
      allocator_free(allocator, &param_types);
      return _pop_scope_return_error(vm, err);
    }
    return_type = (type_t)value_get_data(rt_val);
  } else {
    return_type = (type_t)value_get_data(vm_get_void_type(vm));
  }

  /* 6. pop+dispose temp scope (restores parent, frees temp's registrations) */
  vm_pop_scope(vm);

  /* 7. create callable_type_t with the concrete param types and return type.
   * If the generic function has a rest param (...args), the expanded param_types
   * already contains the individual element types — mark is_variadic so that
   * value_call accepts >= param_count args instead of exact match. */
  bool has_rest_param = false;
  for (size_t i = 0; i < arg_count; i++) {
    cubec_function_argument_t p =
        (cubec_function_argument_t)vec_get(args, i);
    if (p->is_rest) { has_rest_param = true; break; }
  }
  value_t ctv = vm_create_callable_type_value(
      vm, param_types, return_type,
      decl->is_c_variadic || has_rest_param, true, "<generic>");
  allocator_free(allocator, &param_types);
  callable_type_t ct = (callable_type_t)value_get_data(ctv);

  /* 8. create template_scope with generic param → concrete type bindings */
  scope_t template_scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  vec_t param_defs = generic_fn_type_get_params(gt_fn);
  size_t pd_count = vec_get_size(param_defs);
  size_t argv_idx = 0;
  for (size_t i = 0; i < pd_count; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    const char *pname = generic_param_get_name(gp);
    if (generic_param_is_rest(gp)) {
      /* pack param: collect remaining argv into a tuple type value.
       * Always process pack params even when argv_idx >= argc (empty pack). */
      vec_init_t etvi = {.auto_dispose = false};
      vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_class, &etvi);
      while (argv_idx < argc) {
        if (type_get_kind(value_get_type(argv[argv_idx])) == TYPE_KIND_TYPE) {
          type_t t = (type_t)value_get_data(argv[argv_idx]);
          vec_push(element_types, t);
        }
        argv_idx++;
      }
      tuple_type_t tt = tuple_type_create(allocator, element_types, true);
      allocator_free(allocator, &element_types);
      vec_push(vm_get_types(vm), tt); /* register so vm_dispose frees it */
      value_t pack_val = vm_create_value_ref(vm, type_type, (type_t)tt, pname);
      name_t n = name_create(template_scope->allocator, pack_val);
      char *owned = cstring_clone(template_scope->allocator, pname);
      strmap_insert(template_scope->names, owned, n);
      allocator_free(template_scope->allocator, &owned);
    } else {
      if (argv_idx >= argc)
        break;
      name_t n = name_create(template_scope->allocator, argv[argv_idx]);
      char *owned = cstring_clone(template_scope->allocator, pname);
      strmap_insert(template_scope->names, owned, n);
      allocator_free(template_scope->allocator, &owned);
      argv_idx++;
    }
  }

  /* 9. extract function name */
  const char *fn_name = NULL;
  if (decl->name)
    fn_name = string_get(((cubec_literal_identifier_t)decl->name)->value);

  /* 10. create ast_func_value with the template_scope.
   *    Pass name=NULL — the instance should NOT use the generic function's
   *    name as its own; the self-reference in closure_scope will be the
   *    generic template value (registered in step 10c), not the instance.
   *    Pass the full declaration node (not just body) — _ast_func_exec
   *    casts af->node to cubec_declaration_function_t to extract
   *    arguments and body. */
  value_t callable_val =
      create_ast_func_value(vm, ct, NULL, (node_t)decl, template_scope);

  /* 10b. bind closure captures into closure_scope */
  if (decl->captures && vec_get_size(decl->captures) > 0) {
    ast_func_t af = (ast_func_t)value_get_data(callable_val);
    scope_t closure = func_get_closure_scope((func_t)af);
    size_t cap_count = vec_get_size(decl->captures);
    for (size_t ci = 0; ci < cap_count; ci++) {
      cubec_function_capture_t cap =
          (cubec_function_capture_t)vec_get(decl->captures, ci);
      if (!cap || !cap->identifier)
        continue;
      const char *cap_name = string_get(
          ((cubec_literal_identifier_t)cap->identifier)->value);
      name_t found = scope_lookup(vm_get_current_scope(vm), cap_name);
      if (!found || !found->ref) {
        /* clone the callable_val before returning error so it gets cleaned up */
        scope_t orig_scope2 = vm_get_current_scope(vm);
        scope_t gt_scope2 = generic_fn_type_get_scope(gt_fn);
        vm_set_scope(vm, gt_scope2);
        (void)value_clone(vm, callable_val);
        vm_set_scope(vm, orig_scope2);
        return create_exception_value(vm,
            "generic function '%s': closure capture '%s' not found in scope",
            fn_name ? fn_name : "<anonymous>", cap_name);
      }
      /* clone the captured value into closure_scope */
      scope_t prev = vm_set_scope(vm, closure);
      value_t cloned = value_clone(vm, found->ref);
      vm_set_scope(vm, prev);
      if (value_is_abnormal(cloned)) {
        scope_t orig_scope2 = vm_get_current_scope(vm);
        scope_t gt_scope2 = generic_fn_type_get_scope(gt_fn);
        vm_set_scope(vm, gt_scope2);
        (void)value_clone(vm, callable_val);
        vm_set_scope(vm, orig_scope2);
        return cloned;
      }
      name_t n = name_create(closure->allocator, cloned);
      char *owned = cstring_clone(closure->allocator, cap_name);
      strmap_insert(closure->names, owned, n);
      allocator_free(closure->allocator, &owned);
    }
  }

  /* 10c. Override self-reference in closure_scope with the generic template.
   * create_ast_func_value already registered an instance callable as self-ref
   * (type=callable_type_t), but generic function bodies need to find the
   * generic template (type=generic_fn_type_t) so recursive calls trigger
   * re-instantiation with potentially different type arguments.
   * The generic template value is a global resource (lifecycle managed by
   * vm->cfuncs), so we create a new value_t with borrowed data (own=false)
   * instead of cloning. */
  if (fn_name) {
    ast_func_t af = (ast_func_t)value_get_data(callable_val);
    scope_t closure = func_get_closure_scope((func_t)af);
    scope_t gt_scope = generic_fn_type_get_scope(gt_fn);
    name_t gen_name = scope_lookup(gt_scope, fn_name);
    if (gen_name && gen_name->ref) {
      value_t src = gen_name->ref;
      value_t self_ref = value_create(allocator, value_get_type(src),
                                      value_get_data(src), false);
      vec_push(closure->values, self_ref);
      name_t n = name_create(closure->allocator, self_ref);
      char *owned = cstring_clone(closure->allocator, fn_name);
      strmap_insert(closure->names, owned, n);
      allocator_free(closure->allocator, &owned);
    }
  }

  /* 11. clone the callable value into the generic fn's isolated scope */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_fn_type_get_scope(gt_fn);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, callable_val);
  vm_set_scope(vm, orig_scope);

  /* 12. vm_pop_scope already disposed temp scope */

  if (value_is_abnormal(instance))
    return instance;

  /* 12b. Type-check function body (shadow execution).
   * Only at top-level instantiation (not during nested calls from within
   * another function's execution). current_func being set indicates we
   * are inside a function execution (including shadow).
   * Cache entry is added first so recursive generic functions find their
   * own instance. The shadow execution is safe because:
   * - expression_call returns shadow directly in shadow mode
   * - expression_subscript returns shadow for generic instantiation
   * - _ast_func_exec returns shadow for nested shadow calls */
  if (!vm_get_current_func(vm)) {
    /* build cache entry first (borrows both params vec + instance) */
    vec_init_t vi2 = {.auto_dispose = false};
    vec_t params_vec2 = (vec_t)allocator_create(allocator, &g_vec_class, &vi2);
    for (size_t i = 0; i < argc; i++)
      vec_push(params_vec2, argv[i]);
    generic_instance_t gi2 =
        generic_instance_create(allocator, params_vec2, instance);
    vec_push(generic_fn_type_get_instances(gt_fn), gi2);

    value_t check_result = ast_func_check(vm, instance);
    if (value_is_abnormal(check_result)) {
      /* Remove the cache entry on check failure to avoid poisoning */
      vec_t instances = generic_fn_type_get_instances(gt_fn);
      size_t last = vec_get_size(instances);
      if (last > 0) {
        generic_instance_t last_gi =
            (generic_instance_t)vec_get(instances, last - 1);
        if (generic_instance_get_instance(last_gi) == instance) {
          vec_remove(instances, last - 1);
          allocator_free(allocator, &gi2);
          allocator_free(allocator, &params_vec2);
        }
      }
      return check_result;
    }

    return instance;
  }

  /* 13. build cache entry for nested instantiation (borrows both params vec + instance) */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);

  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_push(generic_fn_type_get_instances(gt_fn), gi);

  return instance;
}
