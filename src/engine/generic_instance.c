#include "core/string.h"
#include "core/vec.h"
#include "cubec/declaration_function.h"
#include "cubec/declaration_struct.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "cubec/struct_field.h"
#include "engine/ast_func.h"
#include "engine/callable_type.h"
#include "engine/diagnostic.h"
#include "engine/exception_type.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "engine/generic_type.h"
#include "engine/scope.h"
#include "engine/struct_type.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "run/run.h"
#include <stdbool.h>
#include <string.h>

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

  vec_t param_defs = generic_type_get_params(gt);
  for (size_t i = 0; i < argc; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    const char *pname = generic_param_get_name(gp);

    /* bind name -> concrete value (borrowing: value owned by caller's scope).
     * Do NOT push to temp->values — that would double-free on scope_dispose. */
    name_t name = name_create(temp->allocator, argv[i]);
    strmap_insert(temp->names, pname, name);
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

  /* 2. get AST node */
  cubec_declaration_struct_t decl =
      (cubec_declaration_struct_t)generic_type_get_node(gt);
  if (!decl)
    return create_exception_value(vm, "generic struct '%s' has no AST node",
                                  type_get_name(self_type));

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. create the concrete struct type (registers on temp scope) */
  const char *base_name = type_get_name(self_type);
  value_t type_val =
      vm_create_struct_type_value(vm, base_name, false, "<builtin>");

  /* 5. iterate members, add fields */
  vec_t members = decl->members;
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
      vm_pop_scope(vm);
      return field_type_val;
    }

    /* field type expression must produce a type value */
    if (type_get_kind(value_get_type(field_type_val)) != TYPE_KIND_TYPE) {
      vm_pop_scope(vm);
      return create_exception_value(
          vm, "struct field '%s' type expression must produce a type, got '%s'",
          field_name, type_get_name(value_get_type(field_type_val)));
    }

    vm_struct_add_field(vm, type_val, field_name, field_type_val,
                        field->is_pub);
  }

  /* 6. seal */
  vm_struct_seal(vm, type_val);

  /* 7. clone the sealed struct type value into the generic's isolated scope
   *    BEFORE popping temp scope. vm_pop_scope disposes temp, which frees
   *    type_val — so we must clone while type_val is still alive. */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_type_get_scope(gt);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, type_val);
  vm_set_scope(vm, orig_scope);

  /* 8. pop+dispose temp scope (restores parent, frees temp's registrations) */
  vm_pop_scope(vm);

  if (value_is_abnormal(instance))
    return instance;

  /* 10. build cache entry (borrows both params vec + instance).
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
    return create_exception_value(vm, "generic type '%s' has no AST node",
                                  type_get_name(self_type));
  node_t type_expr = (node_t)node;

  /* 3. bind parameters in a temporary scope (child of current) */
  (void)_bind_params(vm, gt, argc, argv);

  /* 4. evaluate the type expression with parameter substitution */
  value_t result = run_expression(vm, type_expr, false);

  if (value_is_abnormal(result)) {
    vm_pop_scope(vm);
    return create_exception_value(vm, "type expression evaluation failed");
  }

  /* result must be a type value */
  if (type_get_kind(value_get_type(result)) != TYPE_KIND_TYPE) {
    vm_pop_scope(vm);
    return create_exception_value(
        vm, "type alias expression must produce a type, got '%s'",
        type_get_name(value_get_type(result)));
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
    return create_exception_value(vm, "generic fn '%s' has no AST node",
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
      vm_pop_scope(vm);
      allocator_free(allocator, &param_types);
      return type_val;
    }
    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
      vm_pop_scope(vm);
      allocator_free(allocator, &param_types);
      return create_exception_value(vm,
                                    "generic function parameter type "
                                    "expression must produce a type, got '%s'",
                                    type_get_name(value_get_type(type_val)));
    }
    type_t pt = (type_t)value_get_data(type_val);
    vec_push(param_types, pt); /* borrowed: types managed by vm->types */
  }

  /* 5. evaluate return type expression */
  type_t return_type;
  if (decl->return_type) {
    value_t rt_val = run_expression(vm, decl->return_type, false);
    if (value_is_abnormal(rt_val)) {
      vm_pop_scope(vm);
      allocator_free(allocator, &param_types);
      return rt_val;
    }
    if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
      vm_pop_scope(vm);
      allocator_free(allocator, &param_types);
      return create_exception_value(vm,
                                    "generic function return type expression "
                                    "must produce a type, got '%s'",
                                    type_get_name(value_get_type(rt_val)));
    }
    return_type = (type_t)value_get_data(rt_val);
  } else {
    return_type = (type_t)value_get_data(vm_get_void_type(vm));
  }

  /* 6. pop+dispose temp scope (restores parent, frees temp's registrations) */
  vm_pop_scope(vm);

  /* 7. create callable_type_t with the concrete param types and return type */
  value_t ctv = vm_create_callable_type_value(
      vm, param_types, return_type, decl->is_c_variadic, true, "<generic>");
  allocator_free(allocator, &param_types);
  callable_type_t ct = (callable_type_t)value_get_data(ctv);

  /* 8. create template_scope with generic param → concrete type bindings */
  scope_t template_scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);
  vec_t param_defs = generic_fn_type_get_params(gt_fn);
  for (size_t i = 0; i < argc; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    const char *pname = generic_param_get_name(gp);
    name_t n = name_create(template_scope->allocator, argv[i]);
    char *owned = cstring_clone(template_scope->allocator, pname);
    strmap_insert(template_scope->names, owned, n);
    allocator_free(template_scope->allocator, &owned);
  }

  /* 9. extract function name */
  const char *fn_name = NULL;
  if (decl->name)
    fn_name = string_get(((cubec_literal_identifier_t)decl->name)->value);

  /* 10. create ast_func_value with the template_scope.
   *    Pass the full declaration node (not just body) — _ast_func_exec
   *    casts af->node to cubec_declaration_function_t to extract
   *    arguments and body. */
  value_t callable_val =
      create_ast_func_value(vm, ct, fn_name, (node_t)decl, template_scope);

  /* 11. clone the callable value into the generic fn's isolated scope */
  scope_t orig_scope = vm_get_current_scope(vm);
  scope_t gt_scope = generic_fn_type_get_scope(gt_fn);
  vm_set_scope(vm, gt_scope);
  value_t instance = value_clone(vm, callable_val);
  vm_set_scope(vm, orig_scope);

  /* 12. vm_pop_scope already disposed temp scope */

  if (value_is_abnormal(instance))
    return instance;

  /* 13. build cache entry (borrows both params vec + instance) */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params_vec = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (size_t i = 0; i < argc; i++)
    vec_push(params_vec, argv[i]);

  generic_instance_t gi =
      generic_instance_create(allocator, params_vec, instance);
  vec_push(generic_fn_type_get_instances(gt_fn), gi);

  return instance;
}
