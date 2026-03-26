#include "ast/expression_group.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_group(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  node = cubec_read_ast_expression(allocator, &current, end);
  if (!node) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid group expression");
    goto onerror;
  }
  if (node->type == CUBEC_NODE_TYPE_ERROR) {
    err = node;
    node = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }

  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid group expression, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}