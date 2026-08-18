#include "run/run.h"
#include "cubec/literal_string.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/str_type.h"
#include "core/string.h"

value_t run_literal_string(vm_t vm, node_t node, bool shadow) {
  cubec_literal_string_t lit = (cubec_literal_string_t)node;
  type_t type = (type_t)value_get_data(vm_get_str_type(vm));

  if (shadow)
    return vm_create_value_shadow(vm, type, NULL, true);

  return create_str_value(vm, string_get(lit->value));
}
