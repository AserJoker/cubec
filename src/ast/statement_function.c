#include "ast/statement_function.h"
#include "ast/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_function(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end,
                                                   const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_FUNCTION);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t function =
      cubec_read_ast_function_declarator(allocator, &current, end, filename);
  if (!function) {
    goto onerror;
  }
  if (function->type == CUBEC_NODE_TYPE_ERROR) {
    err = function;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "function", function);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = function->loc.end;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}