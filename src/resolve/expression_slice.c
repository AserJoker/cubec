#include "resolve/expression_slice.h"
#include "ast/node.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/expression.h"

value_t resolve_expression_slice(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t start = ast_get_child(node, "start");
  ast_node_t end = ast_get_child(node, "end");
  value_t vstart = NULL;
  if (start) {
    vstart = resolve_expression(ctx, start);
    if (vstart->type->kind == TYPE_KIND_ERROR) {
      return vstart;
    }
    if (vstart->type->kind < TYPE_KIND_I8 ||
        vstart->type->kind > TYPE_KIND_U64) {
      return create_comptime_error(ctx, node_get_location(start),
                                   "slice start is not integer");
    }
  } else {
    vstart = create_comptime_void(ctx);
  }
  value_t vend = NULL;
  if (end) {
    vend = resolve_expression(ctx, end);
    if (vend->type->kind == TYPE_KIND_ERROR) {
      return vend;
    }
    if (vend->type->kind < TYPE_KIND_I8 || vend->type->kind > TYPE_KIND_U64) {
      return create_comptime_error(ctx, node_get_location(end),
                                   "slice end is not integer");
    }
  } else {
    vend = create_comptime_void(ctx);
  }
  value_t obj = resolve_expression(ctx, host);
  if (obj->type->kind == TYPE_KIND_ERROR) {
    return obj;
  }
  value_t result = value_slice(obj, ctx, vstart, vend);
  if (result->type->kind == TYPE_KIND_ERROR) {
    result = convert_comptime_error(ctx, node_get_location(node), result);
  }
  return result;
}