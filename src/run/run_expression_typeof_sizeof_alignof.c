#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/integer_type.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"

/* ---- helper: resolve the concrete type_t from an evaluated expression.
 * If the expression produced a type value (TYPE_KIND_TYPE), unwrap it;
 * otherwise use the value's own type. ---- */

static type_t _resolve_type(value_t inner) {
  if (type_get_kind(value_get_type(inner)) == TYPE_KIND_TYPE)
    return (type_t)value_get_data(inner);
  return value_get_type(inner);
}

/* ---- typeof(expr) ---- */

value_t run_expression_typeof(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_expression_typeof_t t = (cubec_expression_typeof_t)node;

  /* typeof never evaluates the inner expression at runtime — it only needs
   * the type. Evaluate in shadow mode to obtain type information without
   * side effects. typeof never returns shadow: the result is always a
   * concrete type value. For type expressions (e.g. typeof(i32)), the inner
   * value is a type value which is unwrapped to yield the concrete type. */
  value_t inner = run_expression(vm, t->expression, true);
  if (value_is_abnormal(inner))
    return inner;

  type_t result_type = _resolve_type(inner);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  return vm_create_value_ref(vm, type_type, (const void *)result_type, NULL);
}

/* ---- sizeof(expr) ---- */

value_t run_expression_sizeof(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)node;

  /* sizeof never evaluates the inner expression at runtime — only its type
   * matters. Evaluate in shadow mode to obtain type information without
   * side effects. sizeof never returns shadow: the size is always a
   * compile-time known concrete u64 value. */
  value_t inner = run_expression(vm, s->expression, true);
  if (value_is_abnormal(inner))
    return inner;

  return create_u64_value(vm, type_get_size(_resolve_type(inner)));
}

/* ---- alignof(expr) ---- */

value_t run_expression_alignof(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_expression_alignof_t a = (cubec_expression_alignof_t)node;

  /* alignof never evaluates the inner expression at runtime — only its type
   * matters. Evaluate in shadow mode to obtain type information without
   * side effects. alignof never returns shadow: the alignment is always a
   * compile-time known concrete u64 value. */
  value_t inner = run_expression(vm, a->expression, true);
  if (value_is_abnormal(inner))
    return inner;

  return create_u64_value(vm, type_get_align(_resolve_type(inner)));
}
