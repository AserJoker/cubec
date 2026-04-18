#include "eval/expression_compute_member.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include <inttypes.h>

value_t eval_expression_compute_member(context_t ctx, ast_node_t node) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field_node = ast_get_child(node, "field");
  value_t host = eval_expression(ctx, host_node);
  if (value_is_error(host)) {
    return host;
  }
  if (value_is_interrupt(host)) {
    return host;
  }
  value_t field = eval_expression(ctx, field_node);
  if (value_is_error(field)) {
    return field;
  }
  if (value_is_interrupt(field)) {
    return field;
  }
  type_t type = value_get_type(host);
  if (value_type_is(field, VALUE_TYPE_STR)) {
    const char *field_name = *(const char **)value_get_data(field);
    value_t val = value_get_field(host, ctx, field_name);
    if (value_is_error(val)) {
      return convert_compile_error(ctx, node, val);
    }
    return val;
  } else {
    type_t field_type = value_get_type(field);
    type_kind_t kind = type_get_kind(field_type);
    uint64_t idx = 0;
    if (kind >= VALUE_TYPE_INT8 && kind <= VALUE_TYPE_INT64) {
      int64_t i = 0;
      value_t val =
          value_safe_convert(field, ctx, context_load_type(ctx, "i64"));
      i = *(int64_t *)value_get_data(val);
      if (i < 0) {
        return create_compile_error(
            ctx, field_node,
            "array index %" PRIdPTR " is before the beginning of the array", i);
      }
      idx = i;
    } else if (kind >= VALUE_TYPE_UINT8 && kind <= VALUE_TYPE_UINT64) {
      value_t val =
          value_safe_convert(field, ctx, context_load_type(ctx, "u64"));
      idx = *(uint64_t *)value_get_data(val);
    } else {
      return create_compile_error(ctx, field_node,
                                  "array subscript is not an integer");
    }
    value_t val = value_get_index(host, ctx, idx);
    if (value_is_error(val)) {
      return convert_compile_error(ctx, node, val);
    }
    return val;
  }
}