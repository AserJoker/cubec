#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "cubec/declaration_pointer.h"

value_t run_declaration_pointer(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;

  /* evaluate pointee type expression */
  value_t pointee_type_val = run_expression(vm, ptr->type, false);
  if (value_is_error(pointee_type_val)) return pointee_type_val;

  /* pointee type expression must produce a type value */
  if (type_get_kind(value_get_type(pointee_type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm, "pointer pointee type expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(pointee_type_val)));
  type_t pointee_type = (type_t)value_get_data(pointee_type_val);

  return vm_create_pointer_type_value(vm, pointee_type, true, ptr->is_volatile);
}
