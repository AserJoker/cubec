#include "resolve/array_declarator.h"
#include "ast/node.h"
#include "engine/arr.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>

value_t resolve_array_declarator(context_t ctx, ast_node_t node) {
  ast_node_t length = ast_get_child(node, "length");
  ast_node_t type = ast_get_child(node, "type");
  value_t vtype = resolve_type(ctx, type);
  if (vtype->type->kind == TYPE_KIND_ERROR) {
    return vtype;
  }
  type_t base_type = *(type_t *)vtype->data;
  value_t vlen = resolve_expression(ctx, length);
  if (!vlen->comptime) {
    return create_comptime_error(ctx, node_get_location(length),
                                 "array length is not comptime");
  }
  uint64_t len = 0;
  if (vlen->type->kind >= TYPE_KIND_I8 && vlen->type->kind <= TYPE_KIND_I64) {
    int64_t ival = integer_get_value(vlen);
    if (ival < 0) {
      return create_comptime_error(ctx, node_get_location(length),
                                   "array length %" PRIdPTR " < 0");
    }
    len = ival;
  } else if (vlen->type->kind >= TYPE_KIND_U8 &&
             vlen->type->kind <= TYPE_KIND_U64) {
    len = unsigned_get_value(vlen);
  } else {
    return create_comptime_error(ctx, node_get_location(length),
                                 "array length is not integer");
  }
  type_t arr_type = create_arr_type(ctx, base_type, len);
  return create_type_value(ctx, arr_type, false, NULL);
}