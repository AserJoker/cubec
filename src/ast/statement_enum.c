#include "ast/statement_enum.h"
#include "ast/enum_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"

cubec_ast_node_t cubec_read_ast_statement_enum(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_ENUM);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t enu =
      cubec_read_ast_enum_declarator(allocator, &current, end);
  if (!enu) {
    goto onerror;
  }
  if (enu->type == CUBEC_NODE_TYPE_ERROR) {
    err = enu;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "enum", enu);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = enu->loc.end;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}