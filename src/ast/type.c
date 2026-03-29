#include "ast/type.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_type(cubec_allocator_t allocator,
                                     cubec_position_t *position,
                                     const char *end, const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_TYPE);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t expr =
      cubec_read_ast_expression18(allocator, &current, end, filename);
  if (!expr) {
    goto onerror;
  }
  if (expr->type == CUBEC_NODE_TYPE_ERROR) {
    err = expr;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", expr);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}