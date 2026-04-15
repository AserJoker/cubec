#include "ast/statement_enum.h"
#include "ast/enum_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"

ast_node_t read_ast_statement_enum(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_ENUM);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t enu = read_ast_enum_declarator(allocator, &current, end, filename);
  if (!enu) {
    goto onerror;
  }
  if (enu->type == CUBEC_NODE_TYPE_ERROR) {
    err = enu;
    goto onerror;
  }
  ast_add_child(allocator, node, "enum", enu);
  err = ast_skip_all(allocator, &current, end, filename);
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
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}