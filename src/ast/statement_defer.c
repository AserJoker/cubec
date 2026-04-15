#include "ast/statement_defer.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "ast/statement_expression.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_defer(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_DEFER);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "defer")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t statement =
      read_ast_statement_block(allocator, &current, end, filename);
  if (!statement) {
    statement =
        read_ast_statement_expression(allocator, &current, end, filename);
  }
  if (!statement) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid defer statement");
    goto onerror;
  }
  if (statement->type == NODE_TYPE_ERROR) {
    err = statement;
    goto onerror;
  }
  ast_add_child(allocator, node, "statement", statement);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}