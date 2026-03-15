#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

static void cubec_ast_decorator_dispose(cubec_ast_decorator_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->expression);
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_decorator_t cubec_create_ast_decorator(cubec_allocator_t allocator) {
  cubec_ast_decorator_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_decorator_t),
                            (cubec_dispose_fn_t)cubec_ast_decorator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_DECORATOR;
  self->expression = NULL;
  return self;
}

cubec_ast_node_t cubec_read_ast_decorator(cubec_allocator_t allocator,
                                          cubec_position_t *position,
                                          const char *end) {
  cubec_ast_decorator_t node = cubec_create_ast_decorator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[' || *(current.offset + 1) != '[') {
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t expression =
      cubec_read_ast_expression2(allocator, &current, end);
  if (!expression) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  node->expression = expression;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']' || *(current.offset + 1) != ']') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid decorator, missing ']]'");
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}