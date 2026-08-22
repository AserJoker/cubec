#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/bool_type.h"
#include "cubec/expression_ternary.h"

/* ---- helper: check if two types are equal via value_equal ---- */

static value_t _check_type_consistency(vm_t vm, type_t then_type,
                                        type_t else_type, bool shadow) {
  /* Fast path: same pointer → definitely equal */
  if (then_type == else_type)
    return create_void_value(vm);

  /* Wrap types as TYPE_KIND_TYPE values and use value_equal */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t then_tv = vm_create_value_ref(vm, type_type, then_type, NULL);
  value_t else_tv = vm_create_value_ref(vm, type_type, else_type, NULL);
  value_t eq = value_equal(vm, then_tv, else_tv);

  if (value_is_abnormal(eq))
    return eq;

  /* value_equal returns a bool value */
  if (!value_is_shadow(eq) && !(*(bool *)value_get_data(eq)))
    return create_exception_value(vm,
        "ternary branches have different types: '%s' and '%s'",
        type_get_name(then_type), type_get_name(else_type));

  return create_void_value(vm);
}

value_t run_expression_ternary(vm_t vm, node_t node, bool shadow) {
  cubec_expression_ternary_t t = (cubec_expression_ternary_t)node;

  /* Evaluate condition */
  value_t cond_val = run_expression(vm, t->condition, shadow);
  if (value_is_abnormal(cond_val))
    return cond_val;

  /* Condition must be safe_cast to bool */
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
  value_t cond_bool = value_safe_cast(vm, cond_val, bool_type);
  if (value_is_abnormal(cond_bool)) {
    if (shadow) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "ternary condition must be convertible to bool");
      return vm_create_value_shadow(vm, bool_type, NULL, true);
    }
    return cond_bool;
  }

  /* Shadow mode: evaluate both branches for type consistency check */
  if (value_is_shadow(cond_bool)) {
    value_t then_val = run_expression(vm, t->consequent, true);
    if (value_is_abnormal(then_val))
      return then_val;

    value_t else_val = run_expression(vm, t->alternate, true);
    if (value_is_abnormal(else_val))
      return else_val;

    /* Both branches must have the same type */
    value_t check = _check_type_consistency(vm, value_get_type(then_val),
                                             value_get_type(else_val), shadow);
    if (value_is_abnormal(check)) {
      if (shadow) {
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "ternary branches have different types: '%s' and '%s'",
                             type_get_name(value_get_type(then_val)),
                             type_get_name(value_get_type(else_val)));
        return vm_create_value_shadow(vm, value_get_type(then_val), NULL, true);
      }
      return check;
    }

    return vm_create_value_shadow(vm, value_get_type(then_val), NULL, true);
  }

  /* Non-shadow: evaluate only the taken branch */
  bool cond_true = *(bool *)value_get_data(cond_bool);

  if (cond_true) {
    value_t then_val = run_expression(vm, t->consequent, shadow);
    if (value_is_abnormal(then_val))
      return then_val;

    /* Type consistency check: evaluate the other branch in shadow mode */
    value_t else_val = run_expression(vm, t->alternate, true);
    if (!value_is_abnormal(else_val)) {
      value_t check = _check_type_consistency(vm, value_get_type(then_val),
                                               value_get_type(else_val), shadow);
      if (value_is_abnormal(check))
        return check;
    }

    return then_val;
  } else {
    value_t else_val = run_expression(vm, t->alternate, shadow);
    if (value_is_abnormal(else_val))
      return else_val;

    /* Type consistency check: evaluate the other branch in shadow mode */
    value_t then_val = run_expression(vm, t->consequent, true);
    if (!value_is_abnormal(then_val)) {
      value_t check = _check_type_consistency(vm, value_get_type(then_val),
                                               value_get_type(else_val), shadow);
      if (value_is_abnormal(check))
        return check;
    }

    return else_val;
  }
}
