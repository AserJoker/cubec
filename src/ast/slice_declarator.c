#include "ast/slice_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
ast_node_t read_ast_slice_declarator(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  node = create_ast_node(allocator, NODE_TYPE_SLICE_DECLARATOR);
  ast_node_t mut =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (mut) {
    if (mut->type == NODE_TYPE_ERROR) {
      err = mut;
      goto onerror;
    } else if (location_is(mut->loc, "const")) {
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
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}