#include "run/run.h"
#include "engine/vm.h"
#include "engine/nil_type.h"

value_t run_literal_nil(vm_t vm, node_t node, bool shadow) {
  (void)node;

  if (shadow) {
    type_t type = (type_t)value_get_data(vm_get_nil_type(vm));
    return vm_create_value_shadow(vm, type, NULL, true);
  }

  return create_nil_value(vm);
}
