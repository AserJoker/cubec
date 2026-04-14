#include "ast/statement_expression.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t
cubec_read_ast_statement_expression(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_EXPRESSION);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t expression =
      cubec_read_ast_expression(allocator, &current, end, filename);
  if (!expression) {
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", expression);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end, filename);
  if (token && token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid expression statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}