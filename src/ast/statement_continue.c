#include "ast/statement_continue.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"

cubec_ast_node_t cubec_read_ast_statement_continue(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end,
                                                   const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_CONTINUE);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "continue")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement, missing ';'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}