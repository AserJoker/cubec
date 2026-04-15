#include "ast/statement_test.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_test(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_TEST);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (!location_is(token->loc, "test")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t name = read_ast_literal_string(allocator, &current, end, filename);
  if (!name) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid test statement, missing name");
    goto onerror;
  }
  if (name->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_add_child(allocator, node, "name", name);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t body =
      read_ast_statement_block(allocator, &current, end, filename);
  if (!body) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid test statement, missing body");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  ast_add_child(allocator, node, "body", body);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}