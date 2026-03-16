
#include "ast/function_argument.h"
#include "ast/decorator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_function_argument_dispose(cubec_ast_function_argument_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->type);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_function_argument_t
cubec_create_ast_function_argument(cubec_allocator_t allocator) {
  cubec_ast_function_argument_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_function_argument_t),
      (cubec_dispose_fn_t)cubec_ast_function_argument_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_FUNCTION_ARGUMENT;
  cubec_ast_set_field(self, allocator, identifier);
  cubec_ast_set_field(self, allocator, type);
  cubec_ast_set_field(self, allocator, decorators);
  self->decorators = cubec_create_ast_list_node(allocator);
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
    cubec_ast_list_node_append(node->decorators, allocator, decorator);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    identifier = cubec_read_ast_literal_symbol(allocator, &current, end);
    if (identifier && !cubec_location_is(identifier->loc, "...")) {
      current = identifier->loc.begin;
      cubec_allocator_free(allocator, identifier);
      identifier = NULL;
    }
  }
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  node->identifier = identifier;
  if (identifier->type != CUBEC_NODE_TYPE_LITERAL_SYMBOL) {
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
    cubec_ast_node_t type = cubec_read_ast_type(allocator, &current, end);
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
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->identifier, &node->super);
  cubec_ast_set_parent(node->type, &node->super);
  cubec_ast_set_parent(node->decorators, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}