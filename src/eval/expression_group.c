#include "eval/expression_group.h"
#include "ast/node.h"
#include "eval/expression.h"
cubec_value_t cubec_eval_expression_group(cubec_context_t ctx,
                                          cubec_ast_node_t node) {
  cubec_ast_node_t expression = cubec_ast_get_child(node, "expression");
  return cubec_eval_expression(ctx, expression);
}