#include "ast/array_declarator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_array_declarator(allocator_t allocator,
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
  node = create_ast_node(allocator, NODE_TYPE_ARRAY_DECLARATOR);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']') {
    ast_node_t length = read_ast_expression(allocator, &current, end, filename);
    if (!length) {
      goto onerror;
    }
    if (length->type == NODE_TYPE_ERROR) {
      err = length;
      goto onerror;
    }
    if (length->type == NODE_TYPE_LITERAL_IDENTIFIER &&
        !location_is(length->loc, "_")) {
      allocator_free(allocator, length);
      goto onerror;
    }
    ast_add_child(allocator, node, "length", length);
  }
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
  ast_node_t item_type =
      read_ast_expression18(allocator, &current, end, filename);
  if (!item_type) {
    goto onerror;
  }
  if (item_type->type == NODE_TYPE_ERROR) {
    err = item_type;
    goto onerror;
  }
  ast_add_child(allocator, node, "item_type", item_type);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}