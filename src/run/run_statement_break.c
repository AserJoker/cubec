#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/interrupt_type.h"
#include "engine/value.h"
#include "cubec/statement_break.h"

value_t run_statement_break(vm_t vm, node_t node, bool shadow) {
  (void)node;
  (void)shadow;
  /* Break produces a BREAK interrupt with void payload.
   * The loop runner intercepts this at the loop boundary. */
  return create_interrupt_value(vm, INTERRUPT_KIND_BREAK,
                               create_void_value(vm));
}
