
#include "ast/function_argument.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_function_argument_dispose(cubec_ast_function_argument_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->type);
}
cubec_ast_function_argument_t
cubec_create_ast_function_argument(cubec_allocator_t allocator) {
  cubec_ast_function_argument_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_function_argument_t),
      (cubec_dispose_fn_t)cubec_ast_function_argument_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_FUNCTION_ARGUMENT;
  self->identifier = NULL;
  self->type = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->decorators = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_function_argument(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_ast_function_argument_t node =
      cubec_create_ast_function_argument(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_list_append(node->decorators, allocator, decorator);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  node->identifier = identifier;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid function argument, missing ':'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t type = cubec_read_ast_expression2(allocator, &current, end);
  if (!type) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid function argument, missing type");
    goto onerror;
  }
  if (type->type == CUBEC_NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  node->type = type;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}