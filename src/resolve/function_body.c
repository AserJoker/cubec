#include "resolve/function_body.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include "resolve/statement_return.h"
#include "resolve/statement_struct.h"

value_t resolve_function_body(context_t ctx, ast_node_t node) {
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    value_t err = NULL;
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      err = resolve_statement_declaration(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
      err = resolve_statement_function(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
      err = resolve_statement_return(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
      err = resolve_statement_struct(ctx, sts);
    } else {
      err = create_comptime_error(ctx, sts, "unsupport statement");
    }
    if (value_is_error(err)) {
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
      }
    }
    if (value_is_interrupt(err)) {
      return err;
    }
  }
  return context_get_undefined(ctx);
}