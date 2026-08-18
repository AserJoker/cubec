#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/value.h"
#include "cubec/program.h"
#include "core/vec.h"

value_t run_program(vm_t vm, node_t node, bool shadow) {
  cubec_program_node_t program = (cubec_program_node_t)node;

  size_t count = vec_get_size(program->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(program->statements, i);
    value_t v = run_statement(vm, stmt, shadow);

    /* interrupt (break/continue/return) — propagate immediately. Do NOT
     * pop scope: the interrupt value references data from the originating
     * scope. The function-level handler will loop vm_pop_scope until the
     * call-site scope is restored. */
    if (value_is_interrupt(v)) return v;

    /* script-mode exception — statements handle shadow-mode exceptions
     * internally (loop pop + diagnostic + void), so only script mode can
     * propagate an exception this far. */
    if (type_get_kind(value_get_type(v)) == TYPE_KIND_EXCEPTION) return v;
  }

  return create_void_value(vm);
}
