#include "ast/ref_declarator.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_ref_declarator(allocator_t allocator, position_t *position,
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
  if (!location_is(kind->loc, "&")) {
    allocator_free(allocator, kind);
    goto onerror;
  }
  allocator_free(allocator, kind);
  node = create_ast_node(allocator, NODE_TYPE_REF_DECLARATOR);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
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