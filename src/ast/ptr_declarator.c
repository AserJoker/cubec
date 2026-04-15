#include "ast/ptr_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_ptr_declarator(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t kind = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!location_is(kind->loc, "*") && !location_is(kind->loc, "[*]")) {
    allocator_free(allocator, kind);
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_PTR_DECLARATOR);
  ast_add_child(allocator, node, "kind", kind);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t decorators = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    ast_node_t item =
        read_ast_literal_identifier(allocator, &current, end, filename);
    if (!item) {
      break;
    }
    if (item->type == NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    if (location_is(item->loc, "volatile") || location_is(item->loc, "const")) {
      ast_add_item(decorators, item);
    } else {
      current = item->loc.begin;
      allocator_free(allocator, item);
      break;
    }
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
  }
  ast_node_t type = read_ast_expression18(allocator, &current, end, filename);
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