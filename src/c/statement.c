#include "c/statement.h"
#include "ast/node_type.h"
#include "c/statement_block.h"
#include "c/statement_declaration.h"
#include "c/statement_return.h"
void c_statement(c_writer_t writer, ast_node_t node) {
  if (node->type == NODE_TYPE_STATEMENT_BLOCK) {
    c_statement_block(writer, node);
  } else if (node->type == NODE_TYPE_STATEMENT_DECLARATION) {
    c_statement_declaration(writer, node);
  } else if (node->type == NODE_TYPE_STATEMENT_RETURN) {
    c_statement_return(writer, node);
  }
}