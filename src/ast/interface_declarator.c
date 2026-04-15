#include "ast/interface_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_interface_declarator(allocator_t allocator,
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
  node = create_ast_node(allocator, NODE_TYPE_INTERFACE_DECLARATOR);
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
  ast_add_child(allocator, node, "args", args);
  if (*current.offset != ')') {
    for (;;) {
      ast_node_t arg = NULL;
      if (!arg) {
        arg = read_ast_literal_symbol(allocator, &current, end, filename);
        if (arg && !location_is(arg->loc, "...")) {
          current = arg->loc.begin;
          allocator_free(allocator, arg);
          arg = NULL;
        }
      }
      if (!arg) {
        arg = read_ast_variable_declarator(allocator, &current, end, filename);
      }
      if (!arg) {
        arg = read_ast_expression18(allocator, &current, end, filename);
      }
      if (!arg) {
        goto onerror;
      }
      if (arg->type == NODE_TYPE_ERROR) {
        goto onerror;
      }
      ast_add_item(args, arg);
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
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  ast_node_t return_type =
      read_ast_expression18(allocator, &current, end, filename);
  if (!return_type) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid interface expression");
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
  if (*current.offset == '{') {
    goto onerror;
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