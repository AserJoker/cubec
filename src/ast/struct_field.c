#include "ast/struct_field.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_struct_field(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STRUCT_FIELD);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t decorators = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    ast_node_t decorator =
        read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    ast_add_item(decorators, decorator);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
  }
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (token) {
    if (token->type == NODE_TYPE_ERROR) {
      err = token;
      goto onerror;
    }
    if (location_is(token->loc, "pub")) {
      ast_add_child(allocator, node, "pub", token);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
    } else {
      current = token->loc.begin;
      allocator_free(allocator, token);
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }

  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct field");
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct field, missing ':'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  token = read_ast_literal_identifier(allocator, &current, end, filename);
  if (token) {
    if (token->type == NODE_TYPE_ERROR) {
      err = token;
      goto onerror;
    }
    if (location_is(token->loc, "const")) {
      ast_add_child(allocator, node, "mut", token);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
    } else {
      current = token->loc.begin;
      allocator_free(allocator, token);
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t type = read_ast_expression18(allocator, &current, end, filename);
  if (!type) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct field, missing ':'");
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct field");
    goto onerror;
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