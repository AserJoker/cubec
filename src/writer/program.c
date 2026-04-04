#include "writer/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "writer/statement_import.h"
void cubec_write_program(FILE *fp, cubec_ast_node_t node,
                         cubec_write_context *ctx) {
  cubec_ast_node_t statements = cubec_ast_get_child(node, "statements");
  size_t size = cubec_ast_get_length(statements);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_ast_node_t node = cubec_ast_get_item(statements, idx);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
      cubec_write_statement_import(fp, node, ctx);
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_STRUCT) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_ENUM) {
    }
  }
}