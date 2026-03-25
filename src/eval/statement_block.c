#include "eval/statement_block.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "eval/statement_expression.h"
cubec_value_t cubec_eval_statement_block(cubec_context_t ctx,
                                         cubec_ast_statement_block_t sts,
                                         const char *filename) {
  cubec_context_push_scope(ctx);
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)sts->statements;
  for (cubec_list_node_t it = cubec_list_get_first(list->items);
       it != cubec_list_get_end(list->items); it = cubec_list_node_next(it)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_EXPRESSION) {
      cubec_value_t err = cubec_eval_statement_expression(
          ctx, (cubec_ast_statement_expression_t)node, filename);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_BLOCK) {
      cubec_value_t err = cubec_eval_statement_block(
          ctx, (cubec_ast_statement_block_t)node, filename);
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