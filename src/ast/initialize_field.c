#include "ast/initialize_field.h"
#include "ast/expression.h"
#include "ast/initialize_list.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
ast_node_t read_ast_initialize_field(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node =
      create_ast_node(allocator, CUBEC_NODE_TYPE_INITIALIZE_FIELD);
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset == '.') {
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    ast_node_t identifier =
        read_ast_literal_identifier(allocator, &current, end, filename);
    if (!identifier) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid initialize list");
      goto onerror;
    }
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    ast_add_child(allocator, node, "identifier", identifier);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    if (*current.offset != '=') {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid initialize list, missing '='");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  ast_node_t initialize =
      read_ast_expression3(allocator, &current, end, filename);
  if (!initialize) {
    initialize = read_ast_initialize_list(allocator, &current, end, filename);
  }
  if (!initialize) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid initialize list");
    goto onerror;
  }
  if (initialize->type == CUBEC_NODE_TYPE_ERROR) {
    err = initialize;
    goto onerror;
  }
  ast_add_child(allocator, node, "initialize", initialize);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}