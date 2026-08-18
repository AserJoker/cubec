#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_wildcard.h"

value_t run_expression_wildcard(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_expression_wildcard_t w = (cubec_expression_wildcard_t)node;
  /* ? in type context produces the wildcard type value.
   * <?> produces the wildcard tuple type value. */
  if (w->is_tuple)
    return vm_get_wildcard_tuple_type(vm);
  return vm_get_wildcard_type(vm);
}
