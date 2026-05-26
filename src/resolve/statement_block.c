#include "resolve/statement_block.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/void.h"
#include "resolve/statement_expression.h"
#include "resolve/statement_function.h"
#include "resolve/statement_return.h"
#include "resolve/statement_struct.h"
value_t resolve_statement_block(context_t ctx, ast_node_t node) {
  ast_node_t statements = ast_get_child(node, "statements");
  context_push_scope(ctx);
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    value_t err = NULL;
    if (sts->type == NODE_TYPE_STATEMENT_BLOCK) {
      err = resolve_statement_block(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
      err = resolve_statement_function(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
      err = resolve_statement_struct(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      err = resolve_statement_struct(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_EXPRESSION) {
      err = resolve_statement_expression(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
      value_t value = resolve_statement_return(ctx, sts);
      if (value->type->kind == TYPE_KIND_ERROR) {
        err = value;
      } else if (ctx->comptime) {
        return value;
      }
    } else {
      err = create_comptime_error(ctx, node_get_location(sts),
                                  "unsupport statement");
    }
    if (err->type->kind == TYPE_KIND_ERROR) {
      if (ctx->comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
      }
    }
  }
  context_pop_scope(ctx);
  return create_comptime_void(ctx);
}