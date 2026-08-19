#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "cubec/declaration_slice.h"

value_t run_declaration_slice(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_slice_t sl = (cubec_declaration_slice_t)node;

  /* evaluate element type expression */
  value_t elem_type_val = run_expression(vm, sl->type, false);
  if (value_is_abnormal(elem_type_val)) return elem_type_val;

  /* element type expression must produce a type value */
  if (type_get_kind(value_get_type(elem_type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm, "slice element type expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(elem_type_val)));
  type_t elem_type = (type_t)value_get_data(elem_type_val);

  /* TODO: handle is_const / is_volatile qualifiers on slice —
   * currently slice_type_create only takes mut; qualifier support
   * can be added when const/volatile slice types are implemented. */
  return vm_create_slice_type_value(vm, elem_type, true);
}
