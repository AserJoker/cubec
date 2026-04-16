#include "eval/statement_return.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/expression.h"
value_t eval_statement_return(context_t ctx, ast_node_t node) {
  ast_node_t value_node = ast_get_child(node, "value");
  value_t value = NULL;
  if (value_node) {
    value = eval_expression(ctx, value_node);
  } else {
    value = context_get_undefined(ctx);
  }
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_interrupt(value)) {
    return value;
  }
  return context_create_interrupt(ctx, value);
}