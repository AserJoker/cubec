#include "ast/expression_spread.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_spread(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_SPREAD);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t symbol =
      cubec_read_ast_literal_symbol(allocator, &current, end, filename);
  if (!symbol) {
    goto onerror;
  }
  if (symbol->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (!cubec_location_is(symbol->loc, "...")) {
    cubec_allocator_free(allocator, symbol);
    goto onerror;
  }
  cubec_allocator_free(allocator, symbol);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t expression =
      cubec_read_ast_expression18(allocator, &current, end, filename);
  if (!expression) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid spread expression");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", expression);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}