#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "cubec/declaration_qualifier.h"

value_t run_declaration_qualifier(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_qualifier_t qual = (cubec_declaration_qualifier_t)node;

  /* evaluate the inner type expression */
  value_t inner_type_val = run_expression(vm, qual->type, false);
  if (value_is_abnormal(inner_type_val)) return inner_type_val;

  /* inner type expression must produce a type value */
  if (type_get_kind(value_get_type(inner_type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm, "qualifier inner type expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(inner_type_val)));
  type_t inner_type = (type_t)value_get_data(inner_type_val);

  /* const qualifier: create a const variant (mut=false) */
  if (qual->is_const) {
    if (!type_is_mut(inner_type))
      return inner_type_val; /* already const — return as-is */

    type_t const_type = type_create_with_mut(vm, inner_type, false);

    /* create a type value wrapping the const type.
     * Use vm_create_value_ref: data is the type_t (ref, not copied). */
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type, const_type, NULL);
  }

  /* volatile qualifier: volatile is recorded but silently ignored at runtime.
   * Return the inner type value unchanged. */
  return inner_type_val;
}
