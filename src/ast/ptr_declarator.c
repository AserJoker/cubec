#include "ast/ptr_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_ptr_declarator(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end,
                                               const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_PTR_DECLARATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t kind =
      cubec_read_ast_literal_symbol(allocator, &current, end, filename);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == CUBEC_NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!cubec_location_is(kind->loc, "*") &&
      !cubec_location_is(kind->loc, "[*]")) {
    cubec_allocator_free(allocator, kind);
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "kind", kind);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t decorators =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    cubec_ast_node_t item =
        cubec_read_ast_literal_identifier(allocator, &current, end, filename);
    if (!item) {
      break;
    }
    if (item->type == CUBEC_NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    if (cubec_location_is(item->loc, "volatile") ||
        cubec_location_is(item->loc, "const")) {
      cubec_ast_add_item(decorators, item);
    } else {
      current = item->loc.begin;
      cubec_allocator_free(allocator, item);
      break;
    }
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t type =
      cubec_read_ast_type(allocator, &current, end, filename);
  if (!type) {
    goto onerror;
  }
  if (type->type == CUBEC_NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "type", type);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}