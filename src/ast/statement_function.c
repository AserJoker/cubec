#include "ast/statement_function.h"
#include "ast/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_function(allocator_t allocator,
                                       position_t *position, const char *end,
                                       const char *filename) {
  ast_node_t node =
      create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_FUNCTION);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t function =
      read_ast_function_declarator(allocator, &current, end, filename);
  if (!function) {
    goto onerror;
  }
  if (function->type == CUBEC_NODE_TYPE_ERROR) {
    err = function;
    goto onerror;
  }
  ast_add_child(allocator, node, "function", function);
  err = ast_skip_all(allocator, &current, end, filename);
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
  allocator_free(allocator, node);
  return err;
}