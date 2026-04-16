#include "resolve/statement_expression.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdio.h>

value_t resolve_statement_expression(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  value_t err = resolve_expression(ctx, expression);
  if (value_is_error(err)) {
    fprintf(stderr, "%s\n", error_get_message(err));
  }
  return context_get_undefined(ctx);
}