#include "ast/array_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_numeric.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_array_declarator(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_ARRAY_DECLARATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t length =
      cubec_read_ast_literal_numeric(allocator, &current, end, filename);
  if (!length) {
    length =
        cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  }
  if (!length) {
    goto onerror;
  }
  if (length->type == CUBEC_NODE_TYPE_ERROR) {
    err = length;
    goto onerror;
  }
  if (length->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER &&
      !cubec_location_is(length->loc, "_")) {
    cubec_allocator_free(allocator, length);
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "length", length);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t item_type =
      cubec_read_ast_type(allocator, &current, end, filename);
  if (!item_type) {
    goto onerror;
  }
  if (item_type->type == CUBEC_NODE_TYPE_ERROR) {
    err = item_type;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "item_type", item_type);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}