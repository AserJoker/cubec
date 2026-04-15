#include "resolve/expression_group.h"
#include "ast/node.h"
#include "resolve/expression.h"
value_t resolve_expression_group(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  return resolve_expression(ctx, expression);
}