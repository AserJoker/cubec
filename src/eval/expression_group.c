#include "eval/expression_group.h"
#include "ast/node.h"
#include "eval/expression.h"
value_t eval_expression_group(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  return eval_expression(ctx, expression);
}
