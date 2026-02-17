#include "ast/function_self.h"
#include "ast/function_argument.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void cubec_ast_function_self_dispose(cubec_ast_function_self_t self,
                                            cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->argument);
}
cubec_ast_function_self_t
cubec_create_ast_function_self(cubec_allocator_t allocator) {
  cubec_ast_function_self_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_function_self_t),
      (cubec_dispose_fn_t)cubec_ast_function_self_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_FUNCTION_SELF;
  self->argument = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_function_self(cubec_allocator_t allocator,
                                              cubec_position_t *position,
                                              const char *end) {
  cubec_ast_function_self_t node = cubec_create_ast_function_self(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t argument =
      cubec_read_ast_function_argument(allocator, &current, end);
  if (!argument) {
    goto onerror;
  }
  if (argument->type == CUBEC_NODE_TYPE_ERROR) {
    err = argument;
    goto onerror;
  }
  node->argument = argument;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
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