#include "resolve/expression_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_expression_member(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t host = resolve_expression(ctx, host_node);
  if (value_is_error(host)) {
    return host;
  }
  if (value_is_interrupt(host)) {
    return host;
  }
  allocator_t allocaotr = context_get_allocator(ctx);
  char *name = location_get(field->loc, allocaotr);
  value_t res = value_get_field(host, ctx, name);
  allocator_free(allocaotr, name);
  if (value_is_error(res)) {
    return convert_comptime_error(ctx, node, res);
  }
  return res;
}