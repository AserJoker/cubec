#include "ast/statement_import.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_import(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  position_t current = *position;
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    return NULL;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  if (!location_is(identifier->loc, "import")) {
    allocator_free(allocator, identifier);
    return NULL;
  }
  allocator_free(allocator, identifier);
  node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_IMPORT);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  identifier = read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    err =
        create_ast_error(allocator, *position, current, filename,
                         "invalid import statement, missing import declarator");
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid import statement, missing 'from'");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    return token;
  }
  if (!location_is(token->loc, "from")) {
    allocator_free(allocator, token);
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid import statement, missing 'from'");
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  token = read_ast_literal_string(allocator, &current, end, filename);
  if (!token) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid import statement, missing source");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  ast_add_child(allocator, node, "source", token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  token = read_ast_literal_symbol(allocator, &current, end, filename);
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !location_is(token->loc, ";")) {
    allocator_free(allocator, token);
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid import statement, missing ';'");
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