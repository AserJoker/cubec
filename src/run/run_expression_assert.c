#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/bool_type.h"
#include "engine/union_type.h"
#include "engine/struct_type.h"
#include "engine/callable_type.h"
#include "cubec/expression_assert.h"

value_t run_expression_assert(vm_t vm, node_t node, bool shadow) {
  cubec_expression_assert_t assert_expr = (cubec_expression_assert_t)node;

  value_t host = run_expression(vm, assert_expr->host, shadow);
  if (value_is_interrupt(host)) return host;
  if (value_is_abnormal(host)) return host;

  /* Duck-typed .! operator:
   * if (host.ok()) host.value()
   * else panic("...", host.error())
   *
   * Shadow mode: use member_call to invoke value() — method existence
   * and parameter compatibility are validated by member_call itself.
   * Error branch: check that .error() is valid at the type level
   * (compile errors reported, panic ignored). */

  if (shadow && value_is_shadow(host)) {
    value_t v = value_member_call(vm, host, "value", 0, NULL);
    if (value_is_abnormal(v)) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           ".! requires result type with value() method, got '%s'",
                           type_get_name(value_get_type(host)));
      return create_void_value(vm);
    }
    return v;
  }

  /* Non-shadow: evaluate .ok() and branch */
  value_t ok_result = value_member_call(vm, host, "ok", 0, NULL);
  if (value_is_interrupt(ok_result)) return ok_result;
  if (value_is_abnormal(ok_result)) return ok_result;

  if (type_get_kind(value_get_type(ok_result)) != TYPE_KIND_BOOL)
    return create_exception_value(vm, ".! requires .ok() to return bool");

  bool is_ok = *(bool *)value_get_data(ok_result);
  if (is_ok) {
    /* Ok: return .value() */
    return value_member_call(vm, host, "value", 0, NULL);
  }

  /* Error: panic with error info */
  value_t err_val = value_member_call(vm, host, "error", 0, NULL);
  if (value_is_interrupt(err_val)) return err_val;
  if (value_is_abnormal(err_val)) return err_val;

  /* Use to_string for a readable error message */
  value_t err_str = value_to_string(vm, err_val);
  const char *msg = "assertion failed";
  if (!value_is_abnormal(err_str) &&
      type_get_kind(value_get_type(err_str)) == TYPE_KIND_STR)
    msg = (const char *)value_get_data(err_str);

  return create_exception_value(vm, "%s", msg);
}
