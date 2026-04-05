#include "ast/function_declarator.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/function_argument.h"
#include "ast/function_argument_rest.h"
#include "ast/function_body.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_function_declarator(cubec_allocator_t allocator,
                                                    cubec_position_t *position,
                                                    const char *end,
                                                    const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_FUNCTION_DECLARATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t decorators =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "decorators", decorators);
  cubec_ast_node_t closure =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "closure", closure);
  cubec_ast_node_t args =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "args", args);
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_add_item(decorators, decorator);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t kind =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (kind) {
    if (kind->type == CUBEC_NODE_TYPE_ERROR) {
      err = kind;
      goto onerror;
    }
    if (!cubec_location_is(kind->loc, "comptime") &&
        !cubec_location_is(kind->loc, "extern")) {
      current = *position;
      cubec_allocator_free(allocator, kind);
    } else {
      cubec_ast_add_child(allocator, node, "kind", kind);
      err = cubec_ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
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
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '<') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    if (*current.offset != '>') {
      for (;;) {
        cubec_ast_node_t id = cubec_read_ast_literal_identifier(
            allocator, &current, end, filename);
        if (!id) {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid function expression");
          goto onerror;
        }
        if (id->type == CUBEC_NODE_TYPE_ERROR) {
          err = id;
          goto onerror;
        }
        cubec_ast_add_item(closure, id);
        err = cubec_ast_skip_all(allocator, &current, end, filename);
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
        err = cubec_ast_skip_all(allocator, &current, end, filename);
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
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (identifier) {
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "identifier", identifier);
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    if (!cubec_location_is(kind->loc, "comptime")) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid function expression, missing '('");
    }
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    for (;;) {
      cubec_ast_node_t arg = NULL;
      if (!arg) {
        arg = cubec_read_ast_function_argument_rest(allocator, &current, end,
                                                    filename);
      }
      if (!arg) {
        arg = cubec_read_ast_function_argument(allocator, &current, end,
                                               filename);
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
      cubec_ast_add_item(args, arg);
      err = cubec_ast_skip_all(allocator, &current, end, filename);
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
      err = cubec_ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
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

  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ':') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t type =
        cubec_read_ast_expression18(allocator, &current, end, filename);
    if (type) {
      if (type->type == CUBEC_NODE_TYPE_ERROR) {
        err = type;
        goto onerror;
      }
      cubec_ast_add_child(allocator, node, "type", type);
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body =
      cubec_read_ast_function_body(allocator, &current, end, filename);
  if (body) {
    if (body->type == CUBEC_NODE_TYPE_ERROR) {
      err = body;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "body", body);
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}