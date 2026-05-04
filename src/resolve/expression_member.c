#include "resolve/expression_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>
value_t resolve_expression_member(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t host = NULL;
  if (host_node) {
    host = resolve_expression(ctx, host_node);
  } else {
    value_t function = context_get_function(ctx);
    type_t type = value_get_type(function);
    if (type_get_kind(type) == TYPE_KIND_FUNCTION) {
      type = function_type_get_type(type);
      host = create_type_value(ctx, type, false, NULL);
    } else {
      function_declar declar = *(function_declar *)value_get_data(function);
      ast_node_t type = ast_get_child(declar->node, "type");
      host = resolve_expression(ctx, type);
    }
  }
  if (value_is_error(host)) {
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