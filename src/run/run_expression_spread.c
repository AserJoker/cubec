#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/tuple_type.h"
#include "engine/scope.h"
#include "cubec/expression_spread.h"
#include "core/vec.h"

value_t run_expression_spread(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;

  /* evaluate the inner expression */
  value_t inner = run_expression(vm, spread->value, false);
  if (value_is_abnormal(inner))
    return inner;

  /* try vtable spread */
  vec_t expanded = value_spread(vm, inner);
  if (expanded) {
    /* Build a tuple type from the expanded type values */
    allocator_t allocator = vm_get_allocator(vm);
    vec_init_t etvi = {.auto_dispose = false};
    vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_class, &etvi);

    size_t count = vec_get_size(expanded);
    for (size_t i = 0; i < count; i++) {
      value_t tv = (value_t)vec_get(expanded, i);
      if (type_get_kind(value_get_type(tv)) != TYPE_KIND_TYPE) {
        allocator_free(allocator, &element_types);
        allocator_free(allocator, &expanded);
        return create_exception_value(vm,
            "spread element must produce a type, got '%s'",
            type_get_name(value_get_type(tv)));
      }
      type_t t = (type_t)value_get_data(tv);
      vec_push(element_types, t); /* borrowed */
    }
    allocator_free(allocator, &expanded);

    /* create tuple type wrapping the expanded types */
    tuple_type_t tt = tuple_type_create(allocator, element_types, true);
    allocator_free(allocator, &element_types);

    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type, (type_t)tt, NULL);
  }

  return create_exception_value(vm,
      "cannot spread value of type '%s'",
      type_get_name(value_get_type(inner)));
}
