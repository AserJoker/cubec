#include "resolve/array_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/array.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <stdbool.h>
value_t resolve_array_declarator(context_t ctx, ast_node_t node) {
  ast_node_t length = ast_get_child(node, "length");
  ast_node_t type = ast_get_child(node, "type");
  if (length->type == NODE_TYPE_LITERAL_IDENTIFIER &&
      location_is(length->loc, "_")) {
    return create_comptime_error(ctx, node, "unable to infer array size");
  }
  value_t vlen = resolve_expression(ctx, length);
  if (!value_is_comptime(vlen)) {
    return create_comptime_error(ctx, length, "array size is not comptime");
  }
  size_t len = 0;
  if (type_get_kind(value_get_type(vlen)) == TYPE_KIND_INTEGER) {
    int64_t i = integer_get_value(vlen);
    if (i <= 0) {
      return create_comptime_error(ctx, length, "invalid array size");
    }
    len = i;
  } else if (type_get_kind(value_get_type(vlen)) == TYPE_KIND_UNSIGNED) {
    len = unsigned_get_value(vlen);
    if (len == 0) {
      return create_comptime_error(ctx, length, "invalid array size");
    }
  } else {
    return create_comptime_error(ctx, length, "invalid array size");
  }
  value_t vtype = resolve_type(ctx, type);
  if (value_is_error(vtype) || value_is_interrupt(vtype)) {
    return vtype;
  }
  type_t base_type = *(type_t *)value_get_data(vtype);
  type_t array_type = create_array_type(ctx, base_type, len);
  return create_type_value(ctx, array_type, false, NULL);
}