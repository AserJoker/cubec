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

ast_node_t read_ast_function_declarator(allocator_t allocator,
                                        position_t *position, const char *end,
                                        const char *filename) {
  ast_node_t node =
      create_ast_node(allocator, CUBEC_NODE_TYPE_FUNCTION_DECLARATOR);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t decorators = create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    ast_node_t decorator =
        read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    ast_add_item(decorators, decorator);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  ast_node_t kind =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (kind) {
    if (kind->type == CUBEC_NODE_TYPE_ERROR) {
      err = kind;
      goto onerror;
    }
    if (!location_is(kind->loc, "comptime") &&
        !location_is(kind->loc, "extern")) {
      current = *position;
      allocator_free(allocator, kind);
    } else {
      ast_add_child(allocator, node, "kind", kind);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "func")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t closure = create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  ast_add_child(allocator, node, "closure", closure);
  if (*current.offset == '<') {
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    if (*current.offset != '>') {
      for (;;) {
        ast_node_t id =
            read_ast_literal_identifier(allocator, &current, end, filename);
        if (!id) {
          err = create_ast_error(allocator, *position, current, filename,
                                 "invalid function expression");
          goto onerror;
        }
        if (id->type == CUBEC_NODE_TYPE_ERROR) {
          err = id;
          goto onerror;
        }
        ast_add_item(closure, id);
        err = ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
        if (*current.offset == '>') {
          break;
        }
        if (*current.offset != ',') {
          err = create_ast_error(allocator, *position, current, filename,
                                 "invalid function expression");
          goto onerror;
        }
        current.offset++;
        current.column++;
        err = ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
      }
    }
    if (*current.offset != '>') {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid function expression");
      goto onerror;
    }
    current.offset++;
    current.column++;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (identifier) {
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    ast_add_child(allocator, node, "identifier", identifier);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t args = create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  ast_add_child(allocator, node, "arguments", args);
  if (*current.offset != '(') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid function expression, missing '('");
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    for (;;) {
      ast_node_t arg = NULL;
      if (!arg) {
        arg =
            read_ast_function_argument_rest(allocator, &current, end, filename);
      }
      if (!arg) {
        arg = read_ast_function_argument(allocator, &current, end, filename);
      }
      if (!arg) {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid function expression");
        goto onerror;
      }
      if (arg->type == CUBEC_NODE_TYPE_ERROR) {
        err = arg;
        goto onerror;
      }
      ast_add_item(args, arg);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid function expression");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid function expression, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid function expression, missing ':'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t type = read_ast_expression18(allocator, &current, end, filename);
  if (type->type == CUBEC_NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t body = read_ast_function_body(allocator, &current, end, filename);
  if (body) {
    if (body->type == CUBEC_NODE_TYPE_ERROR) {
      err = body;
      goto onerror;
    }
    ast_add_child(allocator, node, "body", body);
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}