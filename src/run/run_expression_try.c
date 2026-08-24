#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/bool_type.h"
#include "engine/callable_type.h"
#include "engine/union_type.h"
#include "engine/struct_type.h"
#include "cubec/expression_try.h"

value_t run_expression_try(vm_t vm, node_t node, bool shadow) {
  cubec_expression_try_t try_expr = (cubec_expression_try_t)node;

  value_t host = run_expression(vm, try_expr->host, shadow);
  if (value_is_interrupt(host)) return host;
  if (value_is_abnormal(host)) return host;

  /* Duck-typed .? operator:
   * if (host.ok()) host.value()
   * else return <current_func_return_type>::of_error(host.error())
   *
   * Shadow mode: use member_call to invoke value() — method existence
   * and parameter compatibility are validated by member_call itself.
   * Error branch: check that .error() and of_error are valid at the type
   * level (compile errors reported, interrupts ignored). */

  if (shadow && value_is_shadow(host)) {
    value_t v = value_member_call(vm, host, "value", 0, NULL);
    if (value_is_abnormal(v)) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           ".? requires result type with value() method, got '%s'",
                           type_get_name(value_get_type(host)));
      return create_void_value(vm);
    }
    return v;
  }

  /* Non-shadow: evaluate .ok() and branch */
  value_t ok_result = value_member_call(vm, host, "ok", 0, NULL);
  if (value_is_interrupt(ok_result)) return ok_result;
  if (value_is_abnormal(ok_result)) return ok_result;

  /* ok_result must be bool */
  if (type_get_kind(value_get_type(ok_result)) != TYPE_KIND_BOOL)
    return create_exception_value(vm, ".? requires .ok() to return bool");

  bool is_ok = *(bool *)value_get_data(ok_result);
  if (is_ok) {
    /* Ok: return .value() */
    return value_member_call(vm, host, "value", 0, NULL);
  }

  /* Error: return <current_func_return_type>::of_error(host.error()) */
  value_t err_val = value_member_call(vm, host, "error", 0, NULL);
  if (value_is_interrupt(err_val)) return err_val;
  if (value_is_abnormal(err_val)) return err_val;

  value_t current_fn = vm_get_current_func(vm);
  if (!current_fn)
    return create_exception_value(vm, ".? error propagation outside function");

  type_t callee_type = value_get_type(current_fn);
  if (type_get_kind(callee_type) != TYPE_KIND_CALLABLE)
    return create_exception_value(vm,
        ".? error propagation: current func has no callable type");

  type_t ret_type = callable_type_get_return_type((callable_type_t)callee_type);

  if (type_get_kind(ret_type) != TYPE_KIND_UNION &&
      type_get_kind(ret_type) != TYPE_KIND_STRUCT)
    return create_exception_value(vm,
        ".? error propagation: return type '%s' is not a result type",
        type_get_name(ret_type));

  /* Create a type value for the return type, then call of_error on it */
  value_t ret_type_val = create_type_value(vm, ret_type, NULL, false);
  value_t of_err_fn = value_get_prop(vm, ret_type_val, "of_error");
  if (!of_err_fn || value_is_abnormal(of_err_fn))
    return create_exception_value(vm,
        ".? cannot find of_error on return type '%s'",
        type_get_name(ret_type));

  /* Call of_error(err_val) to construct the error result */
  value_t err_result = value_call(vm, of_err_fn, 1, &err_val);
  if (value_is_interrupt(err_result)) return err_result;
  if (value_is_abnormal(err_result)) return err_result;

  /* Return the error result via interrupt (return statement semantics) */
  return create_interrupt_value(vm, INTERRUPT_KIND_RETURN, err_result);
}
