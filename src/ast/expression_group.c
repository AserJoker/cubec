#include "ast/expression_group.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_expression_group_dispose(cubec_ast_expression_group_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->body);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_group_t
cubec_create_ast_expression_group(cubec_allocator_t allocator) {
  cubec_ast_expression_group_t group = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_group_t),
      (cubec_dispose_fn_t)cubec_ast_expression_group_dispose);
  cubec_ast_node_initialize(allocator, &group->super);
  group->super.type = CUBEC_NODE_TYPE_EXPRESSION_GROUP;
  cubec_ast_set_field(group, allocator, body);
  return group;
}

cubec_ast_node_t cubec_read_ast_expression_group(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_expression_group_t node =
      cubec_create_ast_expression_group(allocator);
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
  node->body = cubec_read_ast_expression(allocator, &current, end);
  if (!node->body) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid group expression");
    goto onerror;
  }
  if (node->body->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->body;
    node->body = NULL;
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
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->body, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}