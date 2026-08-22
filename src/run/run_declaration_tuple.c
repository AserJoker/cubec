#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/tuple_type.h"
#include "engine/scope.h"
#include "cubec/declaration_tuple.h"
#include "core/vec.h"

value_t run_declaration_tuple(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_tuple_t tuple_decl = (cubec_declaration_tuple_t)node;
  allocator_t allocator = vm_get_allocator(vm);

  vec_t element_types_nodes = tuple_decl->element_types;
  size_t elem_count = vec_get_size(element_types_nodes);

  /* Evaluate each element type expression.
   * Spread nodes (...T) expand into multiple types. */
  vec_init_t etvi = {.auto_dispose = false};
  vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_class, &etvi);

  for (size_t i = 0; i < elem_count; i++) {
    node_t elem_node = (node_t)vec_get(element_types_nodes, i);

    if (elem_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
      /* ...T: evaluate the spread operand and expand */
      value_t spread_val = run_expression(vm, elem_node, false);
      if (value_is_abnormal(spread_val)) {
        allocator_free(allocator, &element_types);
        return spread_val;
      }
      vec_t expanded = value_spread(vm, spread_val);
      if (!expanded) {
        allocator_free(allocator, &element_types);
        return create_exception_value(vm,
            "cannot spread value of type '%s' in tuple type expression",
            type_get_name(value_get_type(spread_val)));
      }
      size_t exp_count = vec_get_size(expanded);
      for (size_t j = 0; j < exp_count; j++) {
        value_t tv = (value_t)vec_get(expanded, j);
        if (type_get_kind(value_get_type(tv)) != TYPE_KIND_TYPE) {
          allocator_free(allocator, &element_types);
          allocator_free(allocator, &expanded);
          return create_exception_value(vm,
              "spread element must produce a type, got '%s'",
              type_get_name(value_get_type(tv)));
        }
        type_t t = (type_t)value_get_data(tv);
        vec_push(element_types, t); /* borrowed: types managed by vm->types */
      }
      allocator_free(allocator, &expanded);
    } else {
      /* Normal type expression */
      value_t type_val = run_expression(vm, elem_node, false);
      if (value_is_abnormal(type_val)) {
        allocator_free(allocator, &element_types);
        return type_val;
      }
      if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
        allocator_free(allocator, &element_types);
        return create_exception_value(vm,
            "tuple element type expression must produce a type, got '%s'",
            type_get_name(value_get_type(type_val)));
      }
      type_t t = (type_t)value_get_data(type_val);
      vec_push(element_types, t); /* borrowed: types managed by vm->types */
    }
  }

  /* create the tuple type */
  tuple_type_t tt = tuple_type_create(allocator, element_types, true);
  allocator_free(allocator, &element_types);

  /* register in vm->types for lifecycle management */
  vec_push(vm_get_types(vm), tt);

  /* create a type value wrapping the tuple type */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  return vm_create_value_ref(vm, type_type, (type_t)tt, NULL);
}
