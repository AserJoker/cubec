#include "ast/function_declarator.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/function_argument.h"
#include "ast/function_body.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
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
}
cubec_ast_function_declarator_t
cubec_create_ast_function_declarator(cubec_allocator_t allocator) {
  cubec_ast_function_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_function_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_function_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_FUNCTION_DECLARATOR;
  self->kind = NULL;
  self->identifier = NULL;
  self->body = NULL;
  self->type = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->args = cubec_create_list(allocator, &initialize);
  self->closure = cubec_create_list(allocator, &initialize);
  self->decorators = cubec_create_list(allocator, &initialize);
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
    cubec_list_append(node->decorators, allocator, decorator);
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
        !cubec_location_is(kind->loc, "extern")) {
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
        cubec_list_append(node->closure, allocator, id);
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
      cubec_ast_node_t arg =
          cubec_read_ast_function_argument(allocator, &current, end);
      if (!arg) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid function expression");
        goto onerror;
      }
      if (arg->type == CUBEC_NODE_TYPE_ERROR) {
        err = arg;
        goto onerror;
      }
      cubec_list_append(node->args, allocator, arg);
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
    cubec_ast_node_t type =
        cubec_read_ast_expression2(allocator, &current, end);
    if (!type) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid function expression");
      goto onerror;
    }
    if (type->type == CUBEC_NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    node->type = type;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (!cubec_location_is(node->kind->loc, "extern")) {
    cubec_ast_node_t body =
        cubec_read_ast_function_body(allocator, &current, end);
    if (!body) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid function expression");
      goto onerror;
    }
    if (body->type == CUBEC_NODE_TYPE_ERROR) {
      err = body;
      goto onerror;
    }
    node->body = body;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}