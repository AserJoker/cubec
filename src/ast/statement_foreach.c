#include "ast/statement_foreach.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"

cubec_ast_node_t cubec_read_ast_statement_foreach(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_FOREACH);
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
  if (!cubec_location_is(token->loc, "foreach")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement, missing '('");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t kind =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!kind) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement");
    goto onerror;
  }
  if (kind->type == CUBEC_NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!cubec_location_is(kind->loc, "const") &&
      !cubec_location_is(kind->loc, "let")) {
    current = kind->loc.begin;
    cubec_allocator_free(allocator, kind);
  } else {
    cubec_ast_add_child(allocator, node, "kind", kind);
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement");
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "identifier", identifier);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement, missing ':'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t expression =
      cubec_read_ast_expression(allocator, &current, end, filename);
  if (!expression) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement");
    goto onerror;
  }
  if (expression->type == CUBEC_NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "expression", expression);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body =
      cubec_read_ast_statement(allocator, &current, end, filename);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid foreach statement, missing body");
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