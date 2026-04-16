#include "resolve/function_body.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_expression.h"
#include "resolve/statement_function.h"
#include "resolve/statement_return.h"
#include <stdio.h>

value_t resolve_function_body(context_t ctx, ast_node_t node) {
  context_push_scope(ctx);
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      resolve_statement_declaration(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
      resolve_statement_function(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_EXPRESSION) {
      resolve_statement_expression(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
      resolve_statement_return(ctx, sts);
    } else {
      value_t err = create_compile_error(ctx, sts, "unsupport statement");
      fprintf(stderr, "%s\n", error_get_message(err));
    }
  }
  context_pop_scope(ctx);
  return context_get_undefined(ctx);
}