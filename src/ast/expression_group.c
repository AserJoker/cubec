#include "ast/expression_group.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
ast_node_t read_ast_expression_group(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_GROUP);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t body = read_ast_expression(allocator, &current, end, filename);
  if (!body) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid group expression");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", body);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }

  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid group expression, missing ')'");
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
ast_node_t ast_unwrap_group(ast_node_t node) {
  while (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    node = ast_get_child(node, "expression");
  }
  return node;
}