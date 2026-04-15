#include "ast/expression_compute_member.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_compute_member(allocator_t allocator,
                                              position_t *position,
                                              const char *end,
                                              const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_COMPUTE_MEMBER);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t field = read_ast_expression(allocator, &current, end, filename);
  if (!field) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token, missing field "
                           "for compute member expression");
    goto onerror;
  }
  if (field->type == NODE_TYPE_ERROR) {
    err = field;
    goto onerror;
  }
  ast_add_child(allocator, node, "field", field);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ']') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token, missing ']' for "
                           "compute member expression");
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
