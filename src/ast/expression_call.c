#include "ast/expression_call.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void cubec_ast_expression_call_dispose(cubec_ast_expression_call_t self,
                                              cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->callee);
  cubec_allocator_free(allocator, self->args);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_call_t
cubec_create_ast_expression_call(cubec_allocator_t allocator) {
  cubec_ast_expression_call_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_call_t),
      (cubec_dispose_fn_t)cubec_ast_expression_call_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_EXPRESSION_CALL;
  cubec_ast_set_field(self, allocator, callee);
  cubec_ast_set_field(self, allocator, args);
  self->args = cubec_create_ast_list_node(allocator);
  return self;
}

cubec_ast_node_t cubec_read_ast_expression_call(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_expression_call_t node =
      cubec_create_ast_expression_call(allocator);
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
  if (*current.offset != ')') {
    cubec_ast_node_t initialize_list =
        cubec_read_ast_initialize_list(allocator, &current, end);
    if (initialize_list) {
      if (initialize_list->type == CUBEC_NODE_TYPE_ERROR) {
        err = initialize_list;
        goto onerror;
      }
      cubec_ast_list_node_append(node->args, allocator, initialize_list);
    } else {
      for (;;) {
        cubec_ast_node_t item =
            cubec_read_ast_expression_spread(allocator, &current, end);
        if (!item) {
          item = cubec_read_ast_expression2(allocator, &current, end);
        }
        if (!item) {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
          goto onerror;
        }
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->args, allocator, item);
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
        if (*current.offset == ')') {
          break;
        }
        if (*current.offset != ',') {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
          goto onerror;
        }
        current.column++;
        current.offset++;
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
      }
    }
  }

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  current.column++;
  current.offset++;

  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->args, &node->super);
  cubec_ast_set_parent(node->callee, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}