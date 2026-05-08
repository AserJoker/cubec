#include "ast/statement_import.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
// import [name:literal_identifier] from [source:literal_string];
ast_node_t read_ast_statement_import(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = NULL;
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
  if (!location_is(token->loc, "import")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_STATEMENT_IMPORT);
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    err = create_ast_error(allocator, *position, current, filename,
                           "unexpected or invalid token, missing identifier");
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  token = read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "from")) {
    allocator_free(allocator, token);
    err = create_ast_error(allocator, *position, current, filename,
                           "unexpected or invalid token, missing 'from'");
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t source =
      read_ast_literal_string(allocator, &current, end, filename);
  if (!source) {
    err = create_ast_error(allocator, *position, current, filename,
                           "unexpected or invalid token, missing source");
    goto onerror;
  }
  if (source->type == NODE_TYPE_ERROR) {
    err = source;
    goto onerror;
  }
  ast_add_child(allocator, node, "source", source);

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = create_ast_error(allocator, *position, current, filename,
                           "unexpected or invalid token, missing ';'");
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
  allocator_free(allocator, node);
  return err;
}