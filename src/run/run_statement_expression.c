#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/type.h"
#include "engine/value.h"
#include "cubec/statement_expression.h"

value_t run_statement_expression(vm_t vm, node_t node, bool shadow) {
  cubec_statement_expression_t stmt = (cubec_statement_expression_t)node;

  scope_t scope_before = vm_get_current_scope(vm);
  value_t v = run_expression(vm, stmt->expression, shadow);

  /* interrupt (break/continue/return) — propagate immediately. The
   * function-level handler will loop vm_pop_scope until the call-site
   * scope is restored. Do NOT pop here: the interrupt value references
   * data from the originating scope. */
  if (value_is_interrupt(v)) return v;

  /* exception — statement is the error handler */
  if (type_get_kind(value_get_type(v)) == TYPE_KIND_EXCEPTION) {
    if (shadow) {
      /* shadow mode: loop pop to restore scope, write diagnostic, return void */
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR, node->location,
                           "expression statement error in compile-time check");
      return create_void_value(vm);
    }
    /* script mode: propagate exception — function boundary handles scope */
    return v;
  }

  /* check: expression statement must return void */
  type_t result_type = value_get_type(v);
  if (type_get_kind(result_type) != TYPE_KIND_VOID) {
    if (shadow) {
      /* shadow mode: loop pop to restore scope, write compile error, return void */
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR, node->location,
                           "expression statement discards non-void value of type '%s'",
                           type_get_name(result_type));
      return create_void_value(vm);
    }
    /* script mode: non-void result is an error */
    return create_exception_value(vm,
        "run: expression statement discards non-void value of type '%s'",
        type_get_name(result_type));
  }

  return create_void_value(vm);
}
