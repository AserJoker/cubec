#include "ast/expression_compute_member.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_compute_member(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator,
                               CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t field = cubec_read_ast_expression(allocator, &current, end);
  if (!field) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token, missing field "
                                 "for compute member expression");
    goto onerror;
  }
  if (field->type == CUBEC_NODE_TYPE_ERROR) {
    err = field;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "field", field);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ']') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token, missing ']' for "
                                 "compute member expression");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
