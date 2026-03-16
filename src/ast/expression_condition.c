#include "ast/expression_condition.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_expression_condition_dispose(cubec_ast_expression_condition_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->condition);
  cubec_allocator_free(allocator, self->alternate);
  cubec_allocator_free(allocator, self->consequent);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_condition_t
cubec_create_ast_expression_condition(cubec_allocator_t allocator) {
  cubec_ast_expression_condition_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_condition_t),
      (cubec_dispose_fn_t)cubec_ast_expression_condition_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_EXPRESSION_CONDITION;
  cubec_ast_set_field(self, allocator, condition);
  cubec_ast_set_field(self, allocator, consequent);
  cubec_ast_set_field(self, allocator, alternate);
  return self;
}

cubec_ast_node_t cubec_read_ast_expression_condition(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_expression_condition_t node =
      cubec_create_ast_expression_condition(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;

  cubec_ast_node_t condition =
      cubec_read_ast_expression4(allocator, &current, end);
  if (!condition) {
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  node->condition = condition;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '?') {
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t consequent =
      cubec_read_ast_expression3(allocator, &current, end);
  if (!consequent) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (consequent->type == CUBEC_NODE_TYPE_ERROR) {
    err = consequent;
    goto onerror;
  }
  node->consequent = consequent;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t alternate =
      cubec_read_ast_expression3(allocator, &current, end);
  if (!alternate) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (alternate->type == CUBEC_NODE_TYPE_ERROR) {
    err = alternate;
    goto onerror;
  }
  node->alternate = alternate;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->condition, &node->super);
  cubec_ast_set_parent(node->consequent, &node->super);
  cubec_ast_set_parent(node->alternate, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}