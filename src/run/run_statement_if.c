#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "cubec/statement_if.h"

value_t run_statement_if(vm_t vm, node_t node, bool shadow) {
  cubec_statement_if_t stmt = (cubec_statement_if_t)node;

  /* Evaluate condition */
  value_t cond_val = run_expression(vm, stmt->condition, shadow);
  if (value_is_interrupt(cond_val)) return cond_val;
  if (value_is_abnormal(cond_val))
    return cond_val;

  /* Strict type: condition must be safe_cast to bool */
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
  value_t cond_bool = value_safe_cast(vm, cond_val, bool_type);
  if (value_is_interrupt(cond_bool)) return cond_bool;
  if (value_is_abnormal(cond_bool)) {
    if (shadow) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "if condition must be convertible to bool");
      return create_void_value(vm);
    }
    return cond_bool;
  }

  /* Shadow condition: evaluate both branches for type checking */
  if (value_is_shadow(cond_bool)) {
    /* Shadow mode: evaluate both branches for type checking.
     * Interrupts (return statements) from branches are collected and
     * merged: if both branches return, create a new merged interrupt
     * to signal that all paths through this if return. */
    value_t then_val = run_statement(vm, stmt->then_branch, true);
    if (value_is_abnormal(then_val) && !value_is_interrupt(then_val))
      return then_val;
    value_t else_val = create_void_value(vm);
    if (stmt->else_branch) {
      else_val = run_statement(vm, stmt->else_branch, true);
      if (value_is_abnormal(else_val) && !value_is_interrupt(else_val))
        return else_val;
    }
    bool then_interrupt = value_is_interrupt(then_val);
    bool else_interrupt = value_is_interrupt(else_val);
    if (then_interrupt && else_interrupt) {
      /* Both branches return — create a new merged interrupt.
       * Use the inner value from then branch as the representative
       * return value. */
      value_t inner = interrupt_get_value(then_val);
      return create_interrupt_value(vm, INTERRUPT_KIND_RETURN, inner);
    }
    if (then_interrupt) return then_val;
    if (else_interrupt) return else_val;
    return create_void_value(vm);
  }

  /* Concrete condition: execute one branch */
  bool cond_true = *(bool *)value_get_data(cond_bool);

  value_t result;
  if (cond_true) {
    result = run_statement(vm, stmt->then_branch, shadow);
  } else if (stmt->else_branch) {
    result = run_statement(vm, stmt->else_branch, shadow);
  } else {
    return create_void_value(vm);
  }

  /* interrupt — propagate immediately */
  if (value_is_interrupt(result)) return result;
  /* exception — propagate */
  return result;
}
