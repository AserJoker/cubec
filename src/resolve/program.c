#include "resolve/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include "resolve/statement_import.h"
#include "resolve/statement_struct.h"

value_t resolve_program(context_t ctx, ast_node_t node) {
  ast_node_t statements = ast_get_child(node, "statements");
  scope_t scope = ctx->current;
  context_push_scope(ctx);
  node->scope = ctx->current;
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    value_t err = NULL;
    if (sts->type == NODE_TYPE_STATEMENT_IMPORT) {
      err = resolve_statement_import(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      err = resolve_statement_declaration(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
      err = resolve_statement_function(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
      err = resolve_statement_struct(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_ENUM) {
    } else {
      err = create_comptime_error(ctx, node_get_location(sts),
                                  "unsupport statement");
      if (!ctx->comptime) {
        context_push_error(ctx, err);
        err = NULL;
      }
    }
    if (err && err->type->kind == TYPE_KIND_ERROR) {
      return err;
    }
  }
  ctx->current = scope;
  return create_comptime_void(ctx);
}