#include "run/run.h"
#include "cubec/literal_bool.h"
#include "engine/vm.h"
#include "engine/bool_type.h"

value_t run_literal_bool(vm_t vm, node_t node, bool shadow) {
  cubec_literal_bool_t lit = (cubec_literal_bool_t)node;

  if (shadow) {
    type_t type = (type_t)value_get_data(vm_get_bool_type(vm));
    return vm_create_value_shadow(vm, type, NULL, true);
  }

  return create_bool_value(vm, lit->value);
}
