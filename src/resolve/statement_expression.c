#include "resolve/statement_expression.h"
#include "ast/node.h"
#include "resolve/expression.h"

value_t resolve_statement_expression(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  value_t value = resolve_expression(ctx, expression);
  return value;
}