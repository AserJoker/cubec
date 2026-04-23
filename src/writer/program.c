#include "writer/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "writer/statement_declaration.h"

void write_program(allocator_t allocator, ast_node_t node, string_t out) {
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      write_statement_declaration(allocator, sts, out);
    }
    string_concat(out, allocator, "\n");
  }
}