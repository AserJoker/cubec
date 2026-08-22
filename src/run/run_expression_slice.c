#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/integer_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "cubec/expression_slice.h"
#include <string.h>

/* ---- helper: extract non-negative u64 from an integer value ---- */

static bool _to_u64_non_negative(vm_t vm, value_t val, uint64_t *out) {
  if (value_is_shadow(val))
    return false;

  type_t t = value_get_type(val);
  type_kind_t kind = type_get_kind(t);

  /* For signed types, check that the value is non-negative before
   * converting to u64. A negative index/length is never valid. */
  if (kind >= TYPE_KIND_I8 && kind <= TYPE_KIND_I64) {
    int64_t signed_val = 0;
    uint64_t size = type_get_size(t);
    memcpy(&signed_val, value_get_data(val), (size_t)size);
    if (signed_val < 0)
      return false;
  }

  /* safe_cast to u64 for uniform extraction */
  type_t u64_type = (type_t)value_get_data(vm_get_u64_type(vm));
  value_t cast = value_safe_cast(vm, val, u64_type);
  if (value_is_abnormal(cast))
    return false;

  *out = *(uint64_t *)value_get_data(cast);
  return true;
}

value_t run_expression_slice(vm_t vm, node_t node, bool shadow) {
  cubec_expression_slice_t s = (cubec_expression_slice_t)node;

  /* Evaluate host */
  value_t host = run_expression(vm, s->host, shadow);
  if (value_is_abnormal(host))
    return host;

  /* Check that the host type supports slicing */
  vtable_t vt = type_get_vtable(value_get_type(host));
  if (!vt.slice) {
    if (shadow) {
      /* shadow mode: return a shadow of the same type for type propagation */
      return vm_create_value_shadow(vm, value_get_type(host), NULL, true);
    }
    return create_exception_value(vm,
        "type '%s' does not support slicing",
        type_get_name(value_get_type(host)));
  }

  /* Shadow mode: return a shadow of the same type */
  if (value_is_shadow(host))
    return vm_create_value_shadow(vm, value_get_type(host), NULL, true);

  /* Resolve start (default 0) and length (default: remainder) */
  uint64_t start = 0;

  if (s->start) {
    value_t start_val = run_expression(vm, s->start, false);
    if (value_is_abnormal(start_val))
      return start_val;

    if (!_to_u64_non_negative(vm, start_val, &start))
      return create_exception_value(vm,
          "slice start index must be a non-negative integer, got '%s'",
          type_get_name(value_get_type(start_val)));
  }

  /* Determine length:
   * - If length expression provided: evaluate it
   * - Otherwise: compute from host's length minus start
   *   For array: count = array_count - start
   *   For slice: count = slice_len - start */
  uint64_t count = 0;

  if (s->length) {
    value_t len_val = run_expression(vm, s->length, false);
    if (value_is_abnormal(len_val))
      return len_val;

    if (!_to_u64_non_negative(vm, len_val, &count))
      return create_exception_value(vm,
          "slice length must be a non-negative integer, got '%s'",
          type_get_name(value_get_type(len_val)));
  } else {
    /* Derive count from host's total length minus start */
    type_kind_t host_kind = type_get_kind(value_get_type(host));

    if (host_kind == TYPE_KIND_ARRAY) {
      /* array count comes from the array type itself */
      array_type_t at = (array_type_t)value_get_type(host);
      uint64_t total = array_type_get_count_value(at);
      count = (start <= total) ? (total - start) : 0;
    } else if (host_kind == TYPE_KIND_SLICE) {
      /* slice_data_t.len is the element count (defined in slice_type.h) */
      struct slice_data_t *sd = (struct slice_data_t *)value_get_data(host);
      uint64_t total = sd->len;
      count = (start <= total) ? (total - start) : 0;
    } else {
      return create_exception_value(vm,
          "type '%s' does not support slicing without explicit length",
          type_get_name(value_get_type(host)));
    }
  }

  return value_slice(vm, host, start, count);
}
