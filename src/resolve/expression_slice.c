#include "resolve/expression_slice.h"
#include "ast/node.h"
#include "core/position.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <stdbool.h>
value_t resolve_expression_slice(context_t ctx, ast_node_t node) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t start_node = ast_get_child(node, "start");
  ast_node_t end_node = ast_get_child(node, "end");
  size_t start = 0;
  size_t end = 0;
  value_t host = resolve_expression(ctx, host_node);
  if (value_is_error(host)) {
    return host;
  }
  type_t type = value_get_type(host);
  if (type_get_kind(type) != TYPE_KIND_ARRAY &&
      type_get_kind(type) != TYPE_KIND_SLICE &&
      type_get_kind(type) != TYPE_KIND_PARRAY) {
    return create_comptime_error(ctx, host_node,
                                 "value is not array,slice or ptr array");
  }
  if (start_node) {
    value_t vstart = resolve_expression(ctx, start_node);
    if (value_is_error(vstart)) {
      return vstart;
    }
    type_t type = value_get_type(vstart);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      int64_t ival = integer_get_value(vstart);
      if (ival < 0) {
        return create_error(
            ctx, "index " PRIdPTR " is before the beginning of the array",
            ival);
      }
      start = ival;
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      start = unsigned_get_value(vstart);
    } else {
      return create_error(ctx, "slice start is not an integer");
    }
  }
  if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    if (start > array_type_get_length(type)) {
      return create_error(
          ctx, "array index %" PRIuPTR " is past the end of the array", start);
    }
  }
  if (type_get_kind(type) == TYPE_KIND_SLICE && value_is_comptime(host)) {
    if (start > slice_get_len(host)) {
      return create_error(
          ctx, "array index %" PRIuPTR " is past the end of the slice", start);
    }
  }
  if (end_node) {
    value_t vend = resolve_expression(ctx, end_node);
    if (value_is_error(vend)) {
      return vend;
    }
    type_t type = value_get_type(vend);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      int64_t ival = integer_get_value(vend);
      if (ival < 0) {
        return create_error(
            ctx, "index " PRIdPTR " is before the beginning of the array",
            ival);
      }
      end = ival;
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      end = unsigned_get_value(vend);
    } else {
      return create_error(ctx, "slice start is not an integer");
    }
  } else if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    end = array_type_get_length(type);
  } else if (type_get_kind(type) == TYPE_KIND_SLICE) {
    if (value_is_comptime(host)) {
      end = slice_get_len(host);
    } else {
      end = start + 1;
    }
  } else {
    return create_error(ctx, "cannot infer slice end for ptr array");
  }
  if (end <= start) {
    return create_error(
        ctx, "slice end %" PRIuPTR " greater than start %" PRIuPTR, end, start);
  }
  size_t len = end - start;
  type_t base_type = NULL;
  if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    base_type = array_type_get_type(type);
  } else if (type_get_kind(type) == TYPE_KIND_SLICE) {
    base_type = slice_type_get_type(type);
  } else {
    base_type = ptr_type_get_type(type);
  }
  type_t slice_type = create_slice_type(ctx, base_type);
  if (value_is_comptime(host)) {
    void *data = NULL;
    if (type_get_kind(type) == TYPE_KIND_ARRAY) {
      data = value_get_data(host);
    } else if (type_get_kind(type) == TYPE_KIND_SLICE) {
      data = slice_get_data(host);
    } else {
      data = *(void **)value_get_data(host);
    }
    return create_comptime_slice(ctx, slice_type, data, start, len);
  } else {
    return context_create_value(ctx, slice_type, NULL, false, false, NULL);
  }
}