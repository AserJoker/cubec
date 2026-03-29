#include "ast/statement_test.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_test(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end,
                                               const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_TEST);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "test")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t name =
      cubec_read_ast_literal_string(allocator, &current, end, filename);
  if (!name) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid test statement, missing name");
    goto onerror;
  }
  if (name->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "name", name);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body =
      cubec_read_ast_statement_block(allocator, &current, end, filename);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid test statement, missing body");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "body", body);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}