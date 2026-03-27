#include "ast/statement_import.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_import(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_position_t current = *position;
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    return NULL;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  if (!cubec_location_is(identifier->loc, "import")) {
    cubec_allocator_free(allocator, identifier);
    return NULL;
  }
  cubec_allocator_free(allocator, identifier);
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_IMPORT);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  identifier = cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid import statement, missing import declarator");
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "identifier", identifier);
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing 'from'");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    return token;
  }
  if (!cubec_location_is(token->loc, "from")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing 'from'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_string(allocator, &current, end);
  if (!token) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing source");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "source", token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}