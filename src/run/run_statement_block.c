#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "cubec/statement_block.h"
#include "core/vec.h"

value_t run_statement_block(vm_t vm, node_t node, bool shadow) {
  cubec_statement_block_t block = (cubec_statement_block_t)node;

  /* push a new block scope */
  scope_t scope_before = vm_get_current_scope(vm);
  scope_t inner = scope_create(vm_get_allocator(vm), SCOPE_BLOCK, scope_before, NULL);
  vm_push_scope(vm, inner);

  size_t count = vec_get_size(block->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(block->statements, i);
    value_t v = run_statement(vm, stmt, shadow);

    /* interrupt (break/continue/return) — propagate immediately. Do NOT
     * pop scope: the interrupt value references data from the originating
     * scope. The function-level handler will loop vm_pop_scope until the
     * call-site scope is restored. */
    if (value_is_interrupt(v)) return v;

    /* exception — check after interrupt since value_is_abnormal includes both */
    if (type_get_kind(value_get_type(v)) == TYPE_KIND_EXCEPTION) {
      if (shadow) {
        /* shadow mode: sub-statement should have handled its own error
         * (loop-pop + diagnostic + void), but defensively ensure scope
         * is back to this block's inner scope before continuing */
        while (vm_get_current_scope(vm) != inner)
          vm_pop_scope(vm);
        continue;
      }
      /* script mode: do NOT pop scope — exception value references data
       * in the throwing scope. The caller (function boundary) is
       * responsible for unwinding the scope stack. */
      return v;
    }
  }

  /* normal exit: pop this block's scope */
  vm_pop_scope(vm);
  return create_void_value(vm);
}
