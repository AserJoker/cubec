#include "resolve/expression_compute_member.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_expression_compute_member(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t obj = NULL;
  obj = resolve_expression(ctx, host);
  if (obj->type->kind == TYPE_KIND_ERROR) {
    return obj;
  }
  value_t f = resolve_expression(ctx, field);
  if (f->type->kind == TYPE_KIND_ERROR) {
    return f;
  }
  value_t result = value_get(obj, ctx, f);
  if (result->type->kind == TYPE_KIND_ERROR) {
    return convert_comptime_error(ctx, node_get_location(node), result);
  }
  if (ctx->comptime && !result->comptime) {
    return create_comptime_error(ctx, node_get_location(node),
                                 "value is not comptime");
  }
  return result;
}