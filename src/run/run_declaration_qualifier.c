#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "cubec/declaration_qualifier.h"

value_t run_declaration_qualifier(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_qualifier_t qual = (cubec_declaration_qualifier_t)node;

  /* evaluate the inner type expression */
  value_t inner_type_val = run_expression(vm, qual->type, false);
  if (value_is_error(inner_type_val)) return inner_type_val;

  /* inner type expression must produce a type value */
  if (type_get_kind(value_get_type(inner_type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm, "qualifier inner type expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(inner_type_val)));
  type_t inner_type = (type_t)value_get_data(inner_type_val);

  /* const qualifier: produce a const version of the type */
  if (qual->is_const) {
    /* use value_safe_cast to derive the const variant — the type value
     * wrapping inner_type is cast to its const counterpart. */
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    value_t inner_as_type = value_create(vm_get_allocator(vm), type_type, inner_type, false);
    value_t result = value_safe_cast(vm, inner_as_type, type_type);
    allocator_free(vm_get_allocator(vm), &inner_as_type);
    return result;
  }

  /* volatile qualifier: volatile is recorded but silently ignored at runtime.
   * Return the inner type value unchanged. */
  return inner_type_val;
}
