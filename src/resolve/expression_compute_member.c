#include "resolve/expression_compute_member.h"
#include "ast/node.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "resolve/expression.h"
#include <corecrt_search.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

value_t resolve_expression_compute_member(context_t ctx, ast_node_t node) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field_node = ast_get_child(node, "field");
  value_t host = resolve_expression(ctx, host_node);
  if (value_is_error(host)) {
    return host;
  }
  value_t field = eval_expression(ctx, field_node);
  if (value_is_error(field)) {
    return field;
  }
  type_t type = value_get_type(host);
  if (value_type_is(field, VALUE_TYPE_STR)) {
    const char *field_name = *(const char **)value_get_data(field);
    if (type_get_kind(type) == VALUE_TYPE_STRUCT) {
      struct_field_t field = struct_type_get_field(type, field_name);
      if (field) {
        return context_create_value(ctx, field->type, false, NULL, NULL);
      } else {
        return create_compile_error(ctx, field_node, "no member %s in value",
                                    field_name);
      }
    } else if (type_get_kind(type) == VALUE_TYPE_UNION) {
      union_field_t field = union_type_get_field(type, field_name);
      if (field) {
        return context_create_value(ctx, field->type, false, NULL, NULL);
      } else {
        return create_compile_error(ctx, field_node, "no member %s in value",
                                    field_name);
      }
    } else if (type_get_kind(type) == VALUE_TYPE_TYPE) {
      return value_get_field(host, ctx, field_name);
    } else {
      return create_compile_error(ctx, host_node,
                                  "value is not struct or union");
    }
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
    if (type_get_kind(type) == VALUE_TYPE_ARRAY) {
      if (idx >= array_type_get_length(type)) {
        return create_compile_error(
            ctx, field_node,
            "array index %" PRIdPTR " is past the end of the array", idx);
      }
      return context_create_value(ctx, array_type_get_type(type), false, NULL,
                                  NULL);
    } else if (type_get_kind(type) == VALUE_TYPE_PARRAY) {
      return context_create_value(ctx, ptr_type_get_type(type), false, NULL,
                                  NULL);
    } else {
      return create_compile_error(ctx, host_node, "value is not array-like");
    }
  }
}