#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "cubec/statement_while.h"

value_t run_statement_while(vm_t vm, node_t node, bool shadow) {
  cubec_statement_while_t stmt = (cubec_statement_while_t)node;

  /* Evaluate condition */
  value_t cond_val = run_expression(vm, stmt->condition, shadow);
  if (value_is_interrupt(cond_val)) return cond_val;
  if (value_is_abnormal(cond_val)) return cond_val;

  /* Strict type: condition must be safe_cast to bool */
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
  value_t cond_bool = value_safe_cast(vm, cond_val, bool_type);
  if (value_is_interrupt(cond_bool)) return cond_bool;
  if (value_is_abnormal(cond_bool)) {
    if (shadow) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "while condition must be convertible to bool");
      return create_void_value(vm);
    }
    return cond_bool;
  }

  /* Shadow condition: type-level analysis — evaluate body once */
  if (value_is_shadow(cond_bool)) {
    /* Shadow mode: the loop may execute 0 or more times.
     * Evaluate the body for type checking; interrupts from body
     * are collected (return/break/continue paths through the loop).
     * Break/continue are loop-local — they don't propagate out. */
    value_t body_val = run_statement(vm, stmt->body, true);
    if (value_is_abnormal(body_val) && !value_is_interrupt(body_val))
      return body_val;
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      /* BREAK / CONTINUE are consumed by the loop — return void */
    }
    return create_void_value(vm);
  }

  /* Concrete condition: execute loop */
  bool cond_true = *(bool *)value_get_data(cond_bool);

  while (cond_true) {
    value_t body_val = run_statement(vm, stmt->body, shadow);
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      if (kind == INTERRUPT_KIND_BREAK) break;           /* exit loop */
      if (kind == INTERRUPT_KIND_CONTINUE) goto reeval;  /* skip to condition */
      return body_val;
    }
    if (value_is_abnormal(body_val)) return body_val;

  reeval:
    /* Re-evaluate condition */
    cond_val = run_expression(vm, stmt->condition, shadow);
    if (value_is_interrupt(cond_val)) return cond_val;
    if (value_is_abnormal(cond_val)) return cond_val;

    cond_bool = value_safe_cast(vm, cond_val, bool_type);
    if (value_is_interrupt(cond_bool)) return cond_bool;
    if (value_is_abnormal(cond_bool)) return cond_bool;
    if (value_is_shadow(cond_bool)) {
      /* Shadow in script mode during loop — shouldn't happen for
       * concrete initial condition, but handle defensively */
      cond_true = false;
    } else {
      cond_true = *(bool *)value_get_data(cond_bool);
    }
  }

  return create_void_value(vm);
}
