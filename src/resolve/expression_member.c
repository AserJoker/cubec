#include "resolve/expression_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_expression_member(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t obj = resolve_expression(ctx, host);
  if (obj->type->kind == TYPE_KIND_ERROR) {
    return obj;
  }
  value_t value = NULL;
  if (node_location_is(field, "&")) {
    value = value_addr(obj, ctx);
  } else if (node_location_is(field, "*")) {
    value = value_deref(obj, ctx);
  } else {
    location_t loc = node_get_location(field);
    char *name = location_get(loc, ctx->allocator);
    value = value_get_field(obj, ctx, name);
    allocator_free(ctx->allocator, name);
  }
  if (value->type->kind == TYPE_KIND_ERROR) {
    return convert_comptime_error(ctx, node_get_location(node), value);
  }
  return value;
}