#include "eval/function_body.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/statement_expression.h"
#include "eval/statement_return.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include <stdio.h>
value_t eval_function_body(context_t ctx, ast_node_t node) {
  context_push_scope(ctx);
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    value_t result = NULL;
    if (sts->type == NODE_TYPE_STATEMENT_EXPRESSION) {
      result = eval_statement_expression(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
      result = eval_statement_return(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      result = resolve_statement_declaration(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
      result = resolve_statement_function(ctx, sts);
    } else {
      result = create_compile_error(ctx, sts, "unsupport statement");
    }
    if (value_is_interrupt(result) || value_is_error(result)) {
      return result;
    }
  }
  return context_get_undefined(ctx);
}