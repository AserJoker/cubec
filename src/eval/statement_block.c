#include "eval/statement_block.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/context.h"
#include "eval/statement_expression.h"
cubec_value_t cubec_eval_statement_block(cubec_context_t ctx,
                                         cubec_ast_node_t sts,
                                         const char *filename) {
  cubec_context_push_scope(ctx);
  cubec_ast_node_t statements =
      cubec_map_get(sts->children, "statements", NULL);
  for (size_t idx = 0; idx < cubec_array_get_size(statements->items); idx++) {
    cubec_ast_node_t node = cubec_array_get(statements->items, idx);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_EXPRESSION) {
      cubec_value_t err = cubec_eval_statement_expression(ctx, node, filename);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_BLOCK) {
      cubec_value_t err = cubec_eval_statement_block(ctx, node, filename);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else {
      return cubec_context_create_compile_error(ctx, node, filename,
                                                "Unsupport top statement");
    }
  }
  cubec_context_pop_scope(ctx);
  return ctx->value_undefined;
}