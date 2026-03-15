#include "ast/expression_compute_member.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

static void cubec_ast_expression_compute_member_dispose(
    cubec_ast_expression_compute_member_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->host);
  cubec_allocator_free(allocator, self->field);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_compute_member_t
cubec_create_ast_expression_compute_member(cubec_allocator_t allocator) {
  cubec_ast_expression_compute_member_t compute_member = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_compute_member_t),
      (cubec_dispose_fn_t)cubec_ast_expression_compute_member_dispose);
  cubec_ast_node_initialize(allocator, &compute_member->super);
  compute_member->super.type = CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER;
  compute_member->field = NULL;
  compute_member->host = NULL;
  return compute_member;
}

cubec_ast_node_t cubec_read_ast_expression_compute_member(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_expression_compute_member_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_compute_member(allocator);
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
  node->field = field;
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
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
