#include "ast/expression_template_generator.h"
#include "ast/expression.h"
#include "ast/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

static void cubec_ast_expression_template_generator_dispose(
    cubec_ast_expression_template_generator_t self,
    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->temp);
  cubec_allocator_free(allocator, self->args);
}
cubec_ast_expression_template_generator_t
cubec_create_ast_expression_template_generator(cubec_allocator_t allocator) {
  cubec_ast_expression_template_generator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_template_generator_t),
      (cubec_dispose_fn_t)cubec_ast_expression_template_generator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR;
  self->temp = NULL;
  self->args = NULL;
  return self;
}

cubec_ast_node_t cubec_read_ast_expression_template_generator(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_expression_template_generator_t node =
      cubec_create_ast_expression_template_generator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '@') {
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t temp = cubec_read_ast_expression18(allocator, &current, end);
  if (!temp) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected template generator expression");
    goto onerror;
  }
  if (temp->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (temp->type != CUBEC_NODE_TYPE_EXPRESSION_CALL) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected template generator expression");
    goto onerror;
  }
  cubec_ast_expression_call_t call = (cubec_ast_expression_call_t)temp;
  node->temp = call->callee;
  call->callee = NULL;
  node->args = call->args;
  call->args = NULL;
  cubec_allocator_free(allocator, call);
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}