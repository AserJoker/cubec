#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/interrupt_type.h"
#include "engine/void_type.h"
#include "cubec/statement_return.h"

value_t run_statement_return(vm_t vm, node_t node, bool shadow) {
  cubec_statement_return_t ret = (cubec_statement_return_t)node;

  if (!ret->expression) {
    /* bare return — interrupt with void */
    value_t void_val = create_void_value(vm);
    return create_interrupt_value(vm, INTERRUPT_KIND_RETURN, void_val);
  }

  value_t result = run_expression(vm, ret->expression, shadow);

  /* propagate errors immediately */
  if (value_is_abnormal(result))
    return result;

  /* shadow: return shadow of the expression's type */
  if (shadow && !value_is_shadow(result)) {
    type_t t = value_get_type(result);
    result = vm_create_value_shadow(vm, t, NULL, true);
  }

  return create_interrupt_value(vm, INTERRUPT_KIND_RETURN, result);
}
