#include "resolve/expression_slice.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <stdbool.h>
value_t resolve_expression_slice(context_t ctx, ast_node_t node) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t start_node = ast_get_child(node, "start");
  ast_node_t end_node = ast_get_child(node, "end");
  value_t host = resolve_expression(ctx, host_node);
  if (value_is_error(host)) {
    return host;
  }
  value_t vstart = start_node ? resolve_expression(ctx, start_node)
                              : context_get_undefined(ctx);
  value_t vend =
      end_node ? resolve_expression(ctx, end_node) : context_get_undefined(ctx);
  value_t res = value_slice(host, ctx, vstart, vend);
  if (value_is_error(res)) {
    return convert_comptime_error(ctx, node, res);
  }
  return res;
}