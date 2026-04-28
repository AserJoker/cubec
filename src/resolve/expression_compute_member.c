#include "resolve/expression_compute_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_expression_compute_member(context_t ctx, ast_node_t node) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field_node = ast_get_child(node, "field");
  allocator_t allocator = context_get_allocator(ctx);
  value_t host = resolve_expression(ctx, host_node);
  if (value_is_error(host) || value_is_interrupt(host)) {
    return host;
  }
  ast_node_bind_value(allocator, host_node, host);
  value_t field = resolve_expression(ctx, field_node);
  if (value_is_error(field) || value_is_interrupt(field)) {
    return field;
  }
  ast_node_bind_value(allocator, field_node, field);
  value_t res = value_get(host, ctx, field);
  if (value_is_error(res)) {
    return convert_comptime_error(ctx, node, res);
  }
  return res;
}