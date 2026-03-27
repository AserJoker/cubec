#include "ast/statement_break.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"

cubec_ast_node_t cubec_read_ast_statement_break(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_BREAK);
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
  if (!cubec_location_is(token->loc, "break")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing ';'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}