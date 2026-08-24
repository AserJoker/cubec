#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "cubec/node.h"
#include "cubec/statement_for.h"

value_t run_statement_for(vm_t vm, node_t node, bool shadow) {
  cubec_statement_for_t stmt = (cubec_statement_for_t)node;

  /* ---- Init ---- */
  if (stmt->init) {
    value_t init_val;
    /* Init can be a statement (e.g. var declaration) or an expression
     * (e.g. assignment, comma). Dispatch accordingly. */
    switch (stmt->init->kind) {
    /* Statement kinds that run_statement handles */
    case CUBEC_NODE_STATEMENT_DECLARATION:
    case CUBEC_NODE_STATEMENT_EMPTY:
    case CUBEC_NODE_STATEMENT_EXPRESSION:
    case CUBEC_NODE_STATEMENT_BLOCK:
      init_val = run_statement(vm, stmt->init, shadow);
      break;
    default:
      /* Everything else is an expression node */
      init_val = run_expression(vm, stmt->init, shadow);
      break;
    }
    if (value_is_interrupt(init_val)) return init_val;
    if (value_is_abnormal(init_val)) return init_val;
  }

  /* ---- Condition ---- */
  /* No condition → infinite loop (always true) */
  bool cond_true = true;
  value_t cond_bool = NULL;

  if (stmt->condition) {
    value_t cond_val = run_expression(vm, stmt->condition, shadow);
    if (value_is_interrupt(cond_val)) return cond_val;
    if (value_is_abnormal(cond_val)) return cond_val;

    type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
    cond_bool = value_safe_cast(vm, cond_val, bool_type);
    if (value_is_interrupt(cond_bool)) return cond_bool;
    if (value_is_abnormal(cond_bool)) {
      if (shadow) {
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "for condition must be convertible to bool");
        return create_void_value(vm);
      }
      return cond_bool;
    }

    /* Shadow condition: type-level analysis — evaluate body + increment once */
    if (value_is_shadow(cond_bool)) {
      value_t body_val = run_statement(vm, stmt->body, true);
      if (value_is_abnormal(body_val) && !value_is_interrupt(body_val))
        return body_val;
      if (value_is_interrupt(body_val)) {
        interrupt_kind_t kind = interrupt_get_kind(body_val);
        if (kind == INTERRUPT_KIND_RETURN) return body_val;
        /* BREAK / CONTINUE consumed by loop */
      }
      if (stmt->increment) {
        value_t incr_val = run_expression(vm, stmt->increment, true);
        if (value_is_abnormal(incr_val) && !value_is_interrupt(incr_val))
          return incr_val;
        /* Increment interrupts are loop-local */
      }
      return create_void_value(vm);
    }

    cond_true = *(bool *)value_get_data(cond_bool);
  } else if (shadow) {
    /* No condition (infinite) in shadow mode: evaluate body + increment once */
    value_t body_val = run_statement(vm, stmt->body, true);
    if (value_is_abnormal(body_val) && !value_is_interrupt(body_val))
      return body_val;
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
    }
    if (stmt->increment) {
      value_t incr_val = run_expression(vm, stmt->increment, true);
      if (value_is_abnormal(incr_val) && !value_is_interrupt(incr_val))
        return incr_val;
    }
    return create_void_value(vm);
  }

  /* ---- Loop ---- */
  while (cond_true) {
    value_t body_val = run_statement(vm, stmt->body, shadow);
    if (value_is_interrupt(body_val)) {
      interrupt_kind_t kind = interrupt_get_kind(body_val);
      if (kind == INTERRUPT_KIND_RETURN) return body_val;
      if (kind == INTERRUPT_KIND_BREAK) break;
      if (kind == INTERRUPT_KIND_CONTINUE) goto increment;
      return body_val;
    }
    if (value_is_abnormal(body_val)) return body_val;

  increment:
    /* Increment */
    if (stmt->increment) {
      value_t incr_val = run_expression(vm, stmt->increment, shadow);
      if (value_is_interrupt(incr_val)) return incr_val;
      if (value_is_abnormal(incr_val)) return incr_val;
    }

    /* Re-evaluate condition */
    if (!stmt->condition) {
      cond_true = true; /* no condition → infinite */
    } else {
      value_t cond_val = run_expression(vm, stmt->condition, shadow);
      if (value_is_interrupt(cond_val)) return cond_val;
      if (value_is_abnormal(cond_val)) return cond_val;

      type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
      cond_bool = value_safe_cast(vm, cond_val, bool_type);
      if (value_is_interrupt(cond_bool)) return cond_bool;
      if (value_is_abnormal(cond_bool)) return cond_bool;
      cond_true = *(bool *)value_get_data(cond_bool);
    }
  }

  return create_void_value(vm);
}
