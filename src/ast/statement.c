#include "ast/statement.h"
#include "ast/node.h"
#include "ast/statement_declaration.h"
#include "ast/statement_expression.h"
#include "ast/statement_import.h"

cubec_ast_node_t cubec_read_ast_statement(cubec_allocator_t allocator,
                                          cubec_position_t *position,
                                          const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_statement_declaration(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_import(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_expression(allocator, position, end);
  if (node) {
    return node;
  }
  return node;
}