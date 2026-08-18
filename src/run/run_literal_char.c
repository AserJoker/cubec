#include "run/run.h"
#include "cubec/literal_char.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/integer_type.h"

value_t run_literal_char(vm_t vm, node_t node, bool shadow) {
  cubec_literal_char_t lit = (cubec_literal_char_t)node;
  type_t type = (type_t)value_get_data(vm_get_u8_type(vm));

  if (shadow)
    return vm_create_value_shadow(vm, type, NULL, true);

  return create_u8_value(vm, (uint8_t)lit->value);
}
