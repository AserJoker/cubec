#include "eval/statement_block.h"
#include "eval/statement.h"
cubec_value_t cubec_eval_statement_block(cubec_context_t ctx,
                                         cubec_ast_node_t node) {
  cubec_context_push_scope(ctx);
  cubec_ast_node_t statements = cubec_ast_get_child(node, "statements");
  for (size_t idx = 0; idx < cubec_ast_get_length(statements); idx++) {
    cubec_ast_node_t sts = cubec_ast_get_item(statements, idx);
    cubec_value_t err = cubec_eval_statement(ctx, sts);
    if (cubec_value_is_error(err)) {
      return err;
    }
  }
  cubec_context_pop_scope(ctx);
  return cubec_context_get_undefined(ctx);
}