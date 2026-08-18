#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"

value_t run_literal_undefined(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  (void)node;
  /* undefined is a placeholder consumed by declaration statements;
     it should never be evaluated directly */
  return create_exception_value(vm, "undefined literal");
}
