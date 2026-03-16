#include "ast/function_declarator.h"
#include "ast/decorator.h"
#include "ast/function_argument.h"
#include "ast/function_argument_rest.h"
#include "ast/function_body.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

static void
cubec_ast_function_declarator_dispose(cubec_ast_function_declarator_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->kind);
  cubec_allocator_free(allocator, self->closure);
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->args);
  cubec_allocator_free(allocator, self->type);
  cubec_allocator_free(allocator, self->body);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_function_declarator_t
cubec_create_ast_function_declarator(cubec_allocator_t allocator) {
  cubec_ast_function_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_function_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_function_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_FUNCTION_DECLARATOR;
  cubec_ast_set_field(self, allocator, kind);
  cubec_ast_set_field(self, allocator, identifier);
  cubec_ast_set_field(self, allocator, body);
  cubec_ast_set_field(self, allocator, type);
  cubec_ast_set_field(self, allocator, args);
  cubec_ast_set_field(self, allocator, closure);
  cubec_ast_set_field(self, allocator, decorators);
  self->args = cubec_create_ast_list_node(allocator);
  self->closure = cubec_create_ast_list_node(allocator);
  self->decorators = cubec_create_ast_list_node(allocator);
  return self;
}
cubec_ast_node_t cubec_read_ast_function_declarator(cubec_allocator_t allocator,
                                                    cubec_position_t *position,
                                                    const char *end) {
  cubec_ast_function_declarator_t node =
      cubec_create_ast_function_declarator(allocator);
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
  cubec_ast_node_t kind =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (kind) {
    if (kind->type == CUBEC_NODE_TYPE_ERROR) {
      err = kind;
      goto onerror;
    }
    if (!cubec_location_is(kind->loc, "inline") &&
        !cubec_location_is(kind->loc, "template") &&
        !cubec_location_is(kind->loc, "comptime") &&
        !cubec_location_is(kind->loc, "extern") &&
        !cubec_location_is(kind->loc, "builtin")) {
      current = *position;
      cubec_allocator_free(allocator, kind);
    } else {
      node->kind = kind;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "func")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '<') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    if (*current.offset != '>') {
      for (;;) {
        cubec_ast_node_t id =
            cubec_read_ast_literal_identifier(allocator, &current, end);
        if (!id) {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid function expression");
          goto onerror;
        }
        if (id->type == CUBEC_NODE_TYPE_ERROR) {
          err = id;
          goto onerror;
        }
        cubec_ast_list_node_append(node->closure, allocator, id);
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
        if (*current.offset == '>') {
          break;
        }
        if (*current.offset != ',') {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid function expression");
          goto onerror;
        }
        current.offset++;
        current.column++;
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
      }
    }
    if (*current.offset != '>') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid function expression");
      goto onerror;
    }
    current.offset++;
    current.column++;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (identifier) {
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    node->identifier = identifier;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    if (!cubec_location_is(node->kind->loc, "comptime")) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid function expression, missing '('");
    }
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    for (;;) {
      cubec_ast_node_t arg = NULL;
      if (!arg) {
        arg = cubec_read_ast_function_argument_rest(allocator, &current, end);
      }
      if (!arg) {
        arg = cubec_read_ast_function_argument(allocator, &current, end);
      }
      if (!arg) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid function expression");
        goto onerror;
      }
      if (arg->type == CUBEC_NODE_TYPE_ERROR) {
        err = arg;
        goto onerror;
      }
      cubec_ast_list_node_append(node->args, allocator, arg);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid function expression");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid function expression, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ':') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t type = cubec_read_ast_type(allocator, &current, end);
    if (type) {
      if (type->type == CUBEC_NODE_TYPE_ERROR) {
        err = type;
        goto onerror;
      }
      node->type = type;
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body =
      cubec_read_ast_function_body(allocator, &current, end);
  if (body) {
    if (body->type == CUBEC_NODE_TYPE_ERROR) {
      err = body;
      goto onerror;
    }
    node->body = body;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->kind, &node->super);
  cubec_ast_set_parent(node->identifier, &node->super);
  cubec_ast_set_parent(node->closure, &node->super);
  cubec_ast_set_parent(node->type, &node->super);
  cubec_ast_set_parent(node->args, &node->super);
  cubec_ast_set_parent(node->body, &node->super);
  cubec_ast_set_parent(node->decorators, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}