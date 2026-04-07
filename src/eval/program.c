#include "eval/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
cubec_value_t cubec_eval_program(cubec_context_t ctx, cubec_ast_node_t node) {
  cubec_ast_node_t statements = cubec_ast_get_child(node, "statements");
  cubec_value_t value = NULL;
  for (size_t idx = 0; idx < cubec_ast_get_length(statements); idx++) {
    cubec_ast_node_t statement = cubec_ast_get_item(statements, idx);
    if (statement->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
    } else if (statements->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (statement->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
    } else if (statements->type == CUBEC_NODE_TYPE_STATEMENT_ENUM) {
    } else if (statements->type == CUBEC_NODE_TYPE_STATEMENT_STRUCT) {
    }
  }
  return value;
}