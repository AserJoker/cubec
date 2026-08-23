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
   * Shadow mode: type-level computation — extract the _value field type
   * from the union type and return a shadow of it. No method calls needed
   * since shadow only cares about type-level existence.
   * Error branch: check that .error() is valid at the type level
   * (compile errors reported, panic ignored). */

  if (shadow && value_is_shadow(host)) {
    type_t host_type = value_get_type(host);

    if (type_get_kind(host_type) == TYPE_KIND_UNION) {
      /* Extract _value field type from the union — this is the unwrapped type */
      union_type_t ut = (union_type_t)host_type;
      field_info_t fv = _union_type_find_field(ut, "_value");
      if (!fv)
        return create_exception_value(vm,
            ".! requires type with '_value' field, got '%s'",
            type_get_name(host_type));
      type_t value_type = field_info_get_type(fv);
      return vm_create_value_shadow(vm, value_type, NULL, true);
    }

    /* For other types (e.g. pointer), delegate to the appropriate vtable.
     * Future: pointer .! could auto-deref. */
    return create_exception_value(vm,
        ".! requires union type, got '%s'", type_get_name(host_type));
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
