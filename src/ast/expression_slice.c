#include "ast/expression_slice.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_expression_slice_dispose(cubec_ast_expression_slice_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->host);
  cubec_allocator_free(allocator, self->start);
  cubec_allocator_free(allocator, self->end);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_slice_t
cubec_create_ast_expression_slice(cubec_allocator_t allocator) {
  cubec_ast_expression_slice_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_slice_t),
      (cubec_dispose_fn_t)cubec_ast_expression_slice_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_EXPRESSION_SLICE;
  cubec_ast_set_field(self, allocator, host);
  cubec_ast_set_field(self, allocator, start);
  cubec_ast_set_field(self, allocator, end);
  return self;
}

cubec_ast_node_t cubec_read_ast_expression_slice(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_expression_slice_t node =
      cubec_create_ast_expression_slice(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t start = cubec_read_ast_expression2(allocator, &current, end);
  if (start) {
    if (start->type == CUBEC_NODE_TYPE_ERROR) {
      err = start;
      goto onerror;
    }
    node->start = start;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  cubec_ast_node_t end_index =
      cubec_read_ast_expression2(allocator, &current, end);
  if (end_index) {
    if (end_index->type == CUBEC_NODE_TYPE_ERROR) {
      err = end_index;
      goto onerror;
    }
    node->end = end_index;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ']') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid slice expression, missing ']'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->host, &node->super);
  cubec_ast_set_parent(node->start, &node->super);
  cubec_ast_set_parent(node->end, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}