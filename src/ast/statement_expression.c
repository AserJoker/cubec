#include "ast/statement_expression.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_statement_expression_dispose(cubec_ast_statement_expression_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->expression);
}
cubec_ast_statement_expression_t
cubec_create_ast_statement_expression(cubec_allocator_t allocator) {
  cubec_ast_statement_expression_t sts = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_expression_t),
      (cubec_dispose_fn_t)cubec_ast_statement_expression_dispose);
  cubec_ast_node_initialize(allocator, &sts->super);
  sts->super.type = CUBEC_NODE_TYPE_STATEMENT_EXPRESSION;
  sts->expression = NULL;
  return sts;
}
cubec_ast_node_t cubec_read_ast_statement_expression(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_statement_expression_t node =
      cubec_create_ast_statement_expression(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t expression =
      cubec_read_ast_expression(allocator, &current, end);
  if (!expression) {
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  node->expression = expression;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (token && token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid expression statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}