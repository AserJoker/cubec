#include "eval/statement.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "eval/statement_block.h"
#include "eval/statement_declaration.h"
#include "eval/statement_function.h"

cubec_value_t cubec_eval_statement(cubec_context_t ctx, cubec_ast_node_t node) {
  if (node->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
    return cubec_eval_statement_declaration(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    return cubec_eval_statement_function(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_BLOCK) {
    return cubec_eval_statement_block(ctx, node);
  }
  return cubec_create_compile_error(ctx, node, "unsupport statement");
}