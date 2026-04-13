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

cubec_ast_node_t
cubec_read_ast_interface_declarator(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename) {
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "func")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_INTERFACE_DECLARATOR);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '*') {
    token = cubec_read_ast_literal_symbol(allocator, &current, end, filename);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!cubec_location_is(token->loc, "*")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
      } else {
        cubec_ast_add_child(allocator, node, "ptr", token);
        err = cubec_ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
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
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t args =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "args", args);
  if (*current.offset != ')') {
    for (;;) {
      cubec_ast_node_t arg = NULL;
      if (!arg) {
        arg = cubec_read_ast_literal_symbol(allocator, &current, end, filename);
        if (arg && !cubec_location_is(arg->loc, "...")) {
          current = arg->loc.begin;
          cubec_allocator_free(allocator, arg);
          arg = NULL;
        }
      }
      if (!arg) {
        arg = cubec_read_ast_variable_declarator(allocator, &current, end,
                                                 filename);
      }
      if (!arg) {
        arg = cubec_read_ast_expression18(allocator, &current, end, filename);
      }
      if (!arg) {
        goto onerror;
      }
      if (arg->type == CUBEC_NODE_TYPE_ERROR) {
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
  if (*current.offset != ')') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  cubec_ast_node_t return_type =
      cubec_read_ast_expression18(allocator, &current, end, filename);
  if (!return_type) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid interface expression");
  }
  if (return_type->type == CUBEC_NODE_TYPE_ERROR) {
    err = return_type;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "type", return_type);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
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
  cubec_allocator_free(allocator, node);
  return err;
}