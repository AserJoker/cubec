#include "ast/statement_expression.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_expression(allocator_t allocator,
                                         position_t *position, const char *end,
                                         const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_EXPRESSION);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t expression =
      read_ast_expression(allocator, &current, end, filename);
  if (!expression) {
    goto onerror;
  }
  if (expression->type == NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", expression);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t token =
      read_ast_literal_symbol(allocator, &current, end, filename);
  if (token && token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !location_is(token->loc, ";")) {
    allocator_free(allocator, token);
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid expression statement, missing ';'");
    goto onerror;
  }
  allocator_free(allocator, token);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}