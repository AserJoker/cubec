#include "ast/callable_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_callable_declarator(allocator_t allocator,
                                        position_t *position, const char *end,
                                        const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (!location_is(token->loc, "func")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  node = create_ast_node(allocator, NODE_TYPE_CALLABLE_DECLARATOR);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '*') {
    token = read_ast_literal_symbol(allocator, &current, end, filename);
    if (token) {
      if (token->type == NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!location_is(token->loc, "*")) {
        current = token->loc.begin;
        allocator_free(allocator, token);
      } else {
        ast_add_child(allocator, node, "ptr", token);
        err = ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == NODE_TYPE_ERROR) {
          return err;
        }
      }
    } else {
      goto onerror;
    }
  }
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t args = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "arguments", args);
  if (*current.offset != ')') {
    for (;;) {
      ast_node_t arg = NULL;
      position_t curr = current;
      ast_node_t token =
          read_ast_literal_symbol(allocator, &current, end, filename);
      if (token && location_is(token->loc, "...")) {
        err = ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == NODE_TYPE_ERROR) {
          return err;
        }
        allocator_free(allocator, token);
        arg = create_ast_node(allocator, NODE_TYPE_FUNCTION_ARGUMENT_REST);
      } else {
        if (token) {
          current = token->loc.begin;
          allocator_free(allocator, token);
        }
        arg = create_ast_node(allocator, NODE_TYPE_FUNCTION_ARGUMENT);
      }
      arg->loc.begin = curr;
      ast_add_item(args, arg);
      ast_node_t mut =
          read_ast_literal_identifier(allocator, &current, end, filename);
      if (mut) {
        if (mut->type == NODE_TYPE_ERROR) {
          err = mut;
          goto onerror;
        }
        if (location_is(mut->loc, "const")) {
          ast_add_child(allocator, node, "mut", mut);
          err = ast_skip_all(allocator, &current, end, filename);
          if (err && err->type == NODE_TYPE_ERROR) {
            return err;
          }
        } else {
          current = mut->loc.begin;
          allocator_free(allocator, mut);
        }
      }
      ast_node_t type =
          read_ast_expression_value(allocator, &current, end, filename);
      if (!type) {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid callable argument");
        goto onerror;
      }
      if (type->type == NODE_TYPE_ERROR) {
        err = type;
        goto onerror;
      }
      ast_add_child(allocator, arg, "type", type);
      arg->loc.end = current;
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  if (*current.offset != ')') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '-' || *(current.offset + 1) != '>') {
    goto onerror;
  }
  current.offset += 2;
  current.column += 2;
  ast_node_t return_type =
      read_ast_expression_value(allocator, &current, end, filename);
  if (!return_type) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid callable expression");
  }
  if (return_type->type == NODE_TYPE_ERROR) {
    err = return_type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", return_type);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  current = return_type->loc.end;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}