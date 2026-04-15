#include "ast/expression_spread.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_expression_spread(allocator_t allocator,
                                      position_t *position, const char *end,
                                      const char *filename) {
  ast_node_t node =
      create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_SPREAD);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t symbol =
      read_ast_literal_symbol(allocator, &current, end, filename);
  if (!symbol) {
    goto onerror;
  }
  if (symbol->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (!location_is(symbol->loc, "...")) {
    allocator_free(allocator, symbol);
    goto onerror;
  }
  allocator_free(allocator, symbol);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t expression =
      read_ast_expression18(allocator, &current, end, filename);
  if (!expression) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid spread expression");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", expression);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}