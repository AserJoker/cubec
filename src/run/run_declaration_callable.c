#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/callable_type.h"
#include "cubec/declaration_callable.h"
#include "cubec/expression_spread.h"
#include "core/vec.h"

value_t run_declaration_callable(vm_t vm, node_t node, bool shadow) {
  cubec_declaration_callable_t decl = (cubec_declaration_callable_t)node;
  allocator_t allocator = vm_get_allocator(vm);

  /* 1. evaluate parameter type expressions, expanding spread nodes */
  vec_init_t ptvi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(allocator, &g_vec_class, &ptvi);

  size_t param_count = vec_get_size(decl->parameters);
  for (size_t i = 0; i < param_count; i++) {
    node_t param_node = (node_t)vec_get(decl->parameters, i);

    if (param_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
      /* spread: evaluate inner, call value_spread, push each expanded type */
      cubec_expression_spread_t spread = (cubec_expression_spread_t)param_node;
      value_t inner = run_expression(vm, spread->value, shadow);
      if (value_is_abnormal(inner)) {
        allocator_free(allocator, &param_types);
        return inner;
      }
      vec_t expanded = value_spread(vm, inner);
      if (!expanded) {
        allocator_free(allocator, &param_types);
        return create_exception_value(vm,
            "cannot spread type '%s' in callable parameter list",
            type_get_name(value_get_type(inner)));
      }
      size_t exp_count = vec_get_size(expanded);
      for (size_t j = 0; j < exp_count; j++) {
        value_t tv = (value_t)vec_get(expanded, j);
        if (type_get_kind(value_get_type(tv)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &param_types);
          allocator_free(allocator, &expanded);
          return create_exception_value(vm,
              "callable spread element must produce a type, got '%s'",
              type_get_name(value_get_type(tv)));
        }
        type_t t = (type_t)value_get_data(tv);
        vec_push(param_types, t);
      }
      allocator_free(allocator, &expanded);
    } else {
      /* normal parameter type expression */
      value_t type_val = run_expression(vm, param_node, shadow);
      if (value_is_abnormal(type_val)) {
        allocator_free(allocator, &param_types);
        return type_val;
      }
      if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
        allocator_free(allocator, &param_types);
        return create_exception_value(vm,
            "callable parameter type expression must produce a type, got '%s'",
            type_get_name(value_get_type(type_val)));
      }
      type_t pt = (type_t)value_get_data(type_val);
      vec_push(param_types, pt);
    }
  }

  /* 2. evaluate return type expression */
  type_t return_type;
  if (decl->return_type) {
    value_t rt_val = run_expression(vm, decl->return_type, shadow);
    if (value_is_abnormal(rt_val)) {
      allocator_free(allocator, &param_types);
      return rt_val;
    }
    if (type_get_kind(value_get_type(rt_val)) != TYPE_KIND_TYPE) {
      allocator_free(allocator, &param_types);
      return create_exception_value(vm,
          "callable return type expression must produce a type, got '%s'",
          type_get_name(value_get_type(rt_val)));
    }
    return_type = (type_t)value_get_data(rt_val);
  } else {
    return_type = (type_t)value_get_data(vm_get_void_type(vm));
  }

  /* 3. create callable_type_t */
  value_t ctv = vm_create_callable_type_value(vm, param_types, return_type,
                                               decl->is_c_variadic, true,
                                               "<callable>");
  allocator_free(allocator, &param_types);
  return ctv;
}
