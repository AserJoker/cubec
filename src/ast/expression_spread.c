#include "ast/expression_spread.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_expression_spread_dispose(cubec_ast_expression_spread_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->expression);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_spread_t
cubec_create_ast_expression_spread(cubec_allocator_t allocator) {
  cubec_ast_expression_spread_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_spread_t),
      (cubec_dispose_fn_t)cubec_ast_expression_spread_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_EXPRESSION_SPREAD;
  self->expression = NULL;
  return self;
}

cubec_ast_node_t cubec_read_ast_expression_spread(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_ast_expression_spread_t node =
      cubec_create_ast_expression_spread(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t symbol =
      cubec_read_ast_literal_symbol(allocator, &current, end);
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
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  node->expression = cubec_read_ast_expression18(allocator, &current, end);
  if (!node->expression) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid spread expression");
    goto onerror;
  }
  if (node->expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->expression;
    node->expression = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}