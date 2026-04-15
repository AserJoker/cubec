#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_decorator(allocator_t allocator, position_t *position,
                              const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_DECORATOR);
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '[' || *(current.offset + 1) != '[') {
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t expression =
      read_ast_expression3(allocator, &current, end, filename);
  if (!expression) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", expression);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']' || *(current.offset + 1) != ']') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid decorator, missing ']]'");
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
  allocator_free(allocator, node);
  return err;
}