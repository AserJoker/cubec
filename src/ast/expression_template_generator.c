#include "ast/expression_template_generator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
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
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->args = cubec_create_list(allocator, &initialize);
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
  node->temp = temp;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '(') {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid template generator expression, missing '('");
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_expression2(allocator, &current, end);
      if (!item) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid template generator argument");
        goto onerror;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      cubec_list_append(node->args, allocator, item);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        goto onerror;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(
            allocator, *position, current,
            "Invalid template generator expression, missing ','");
        goto onerror;
      } else {
        current.offset++;
        current.column++;
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
      }
    }
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid template generator expression, missing ')'");
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