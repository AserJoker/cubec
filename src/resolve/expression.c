#include "resolve/expression.h"
#include "ast/node.h"
#include "engine/error.h"

value_t resolve_expression(context_t ctx, ast_node_t node) {
  return create_comptime_error(ctx, node_get_location(node),
                               "unsupport expression");
}