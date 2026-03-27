#include "ast/statement_defer.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "ast/statement_expression.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_defer(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_DEFER);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "defer")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t statement =
      cubec_read_ast_statement_block(allocator, &current, end);
  if (!statement) {
    statement = cubec_read_ast_statement_expression(allocator, &current, end);
  }
  if (!statement) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid defer statement");
    goto onerror;
  }
  if (statement->type == CUBEC_NODE_TYPE_ERROR) {
    err = statement;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "statement", statement);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}