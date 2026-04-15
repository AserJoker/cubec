#include "ast/statement_empty.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_empty(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_EMPTY);
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != ';') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}