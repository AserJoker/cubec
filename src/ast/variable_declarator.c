#include "ast/variable_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_variable_declarator(cubec_allocator_t allocator,
                                                    cubec_position_t *position,
                                                    const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_VARIABLE_DECLARATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "identifier", identifier);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset == ':') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_node_t type = cubec_read_ast_type(allocator, &current, end);
    if (!type) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (type->type == CUBEC_NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "type", type);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset == '=') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_node_t initialize =
        cubec_read_ast_expression2(allocator, &current, end);
    if (!initialize) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (initialize->type == CUBEC_NODE_TYPE_ERROR) {
      err = initialize;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "initialize", initialize);
  }
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}