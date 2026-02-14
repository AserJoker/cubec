#include "ast/expression_comma.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_expression_comma_dispose(cubec_ast_expression_comma_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->current);
  cubec_allocator_free(allocator, self->next);
}
cubec_ast_expression_comma_t
cubec_create_ast_expression_comma(cubec_allocator_t allocator) {
  cubec_ast_expression_comma_t node = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_comma_t),
      (cubec_dispose_fn_t)cubec_ast_expression_comma_dispose);
  cubec_ast_node_initialize(allocator, &node->super);
  node->current = NULL;
  node->next = NULL;
  node->super.type = CUBEC_NODE_TYPE_EXPRESSION_COMMON;
  return node;
}

cubec_ast_node_t cubec_read_ast_expression_comma(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_position_t current = *position;
  cubec_ast_expression_comma_t node = NULL;
  cubec_ast_node_t err = NULL;
  node = cubec_create_ast_expression_comma(allocator);
  node->current = cubec_read_ast_expression2(allocator, &current, end);
  if (!node->current) {
    goto onerror;
  }
  if (node->current->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->current;
    node->current = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  if (*current.offset != ',') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->next = cubec_read_ast_expression1(allocator, &current, end);
  if (!node->next) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid comma expression");
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}