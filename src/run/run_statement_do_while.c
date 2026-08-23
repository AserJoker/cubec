#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "cubec/statement_do_while.h"

value_t run_statement_do_while(vm_t vm, node_t node, bool shadow) {
  cubec_statement_do_while_t stmt = (cubec_statement_do_while_t)node;

  /* do-while: body executes at least once, then check condition */

  /* Shadow mode: evaluate body + condition once for type checking */
  if (shadow) {
    value_t body_val = run_statement(vm, stmt->body, true);
    if (value_is_abnormal(body_val) && !value_is_interrupt(body_val))
      return body_val;
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      /* BREAK / CONTINUE consumed by loop — continue to condition check */
    }

    /* Evaluate condition */
    value_t cond_val = run_expression(vm, stmt->condition, true);
    if (value_is_interrupt(cond_val)) return cond_val;
    if (value_is_abnormal(cond_val)) return cond_val;

    type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
    value_t cond_bool = value_safe_cast(vm, cond_val, bool_type);
    if (value_is_interrupt(cond_bool)) return cond_bool;
    if (value_is_abnormal(cond_bool)) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "do-while condition must be convertible to bool");
      return create_void_value(vm);
    }

    return create_void_value(vm);
  }

  /* Script mode: execute loop */
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));

  for (;;) {
    /* Execute body */
    value_t body_val = run_statement(vm, stmt->body, shadow);
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      if (kind == INTERRUPT_KIND_BREAK) break;
      if (kind == INTERRUPT_KIND_CONTINUE) goto reeval;
      return body_val;
    }
    if (value_is_abnormal(body_val)) return body_val;

  reeval:
    /* Evaluate condition */
    value_t cond_val = run_expression(vm, stmt->condition, shadow);
    if (value_is_interrupt(cond_val)) return cond_val;
    if (value_is_abnormal(cond_val)) return cond_val;

    value_t cond_bool = value_safe_cast(vm, cond_val, bool_type);
    if (value_is_interrupt(cond_bool)) return cond_bool;
    if (value_is_abnormal(cond_bool)) return cond_bool;

    bool cond_true = *(bool *)value_get_data(cond_bool);
    if (!cond_true) break;
  }

  return create_void_value(vm);
}
