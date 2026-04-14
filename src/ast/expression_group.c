#include "ast/expression_group.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
cubec_ast_node_t cubec_read_ast_expression_group(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_GROUP);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t body =
      cubec_read_ast_expression(allocator, &current, end, filename);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid group expression");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", body);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }

  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
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
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_ast_unwrap_group(cubec_ast_node_t node) {
  while (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    node = cubec_ast_get_child(node, "expression");
  }
  return node;
}