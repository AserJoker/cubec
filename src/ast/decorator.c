#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_decorator(cubec_allocator_t allocator,
                                          cubec_position_t *position,
                                          const char *end,
                                          const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_DECORATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[' || *(current.offset + 1) != '[') {
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t expression =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (!expression) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", expression);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']' || *(current.offset + 1) != ']') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid decorator, missing ']]'");
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}