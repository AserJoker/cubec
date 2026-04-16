#include "eval/statement_expression.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/expression.h"

value_t eval_statement_expression(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  value_t value = eval_expression(ctx, expression);
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_interrupt(value)) {
    return value;
  }
  return context_get_undefined(ctx);
}