#include "ast/statement_empty.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_empty(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end,
                                                const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_EMPTY);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
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
  cubec_allocator_free(allocator, node);
  return err;
}