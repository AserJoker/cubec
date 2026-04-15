#include "ast/statement_for.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "ast/statement_declaration.h"
#include "ast/statement_empty.h"
#include "ast/statement_expression.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_for(allocator_t allocator, position_t *position,
                                  const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_FOR);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "for")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing '('");
    goto onerror;
  }
  current.offset++;
  current.column++;
  ast_node_t init =
      read_ast_statement_declaration(allocator, &current, end, filename);
  if (!init) {
    init = read_ast_statement_expression(allocator, &current, end, filename);
  }
  if (!init) {
    init = read_ast_statement_empty(allocator, &current, end, filename);
  }
  if (!init) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid for statement, missing initialize");
    goto onerror;
  }
  if (init->type == CUBEC_NODE_TYPE_ERROR) {
    err = init;
    goto onerror;
  }
  ast_add_child(allocator, node, "init", init);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t conditon =
      read_ast_statement_expression(allocator, &current, end, filename);
  if (!conditon) {
    conditon = read_ast_statement_empty(allocator, &current, end, filename);
  }
  if (!conditon) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid for statement, missing condition");
    goto onerror;
  }
  if (conditon->type == CUBEC_NODE_TYPE_ERROR) {
    err = conditon;
    goto onerror;
  }
  ast_add_child(allocator, node, "condition", conditon);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t after = read_ast_expression(allocator, &current, end, filename);
  if (after) {
    if (after->type == CUBEC_NODE_TYPE_ERROR) {
      err = after;
      goto onerror;
    }
    ast_add_child(allocator, node, "after", after);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t body = read_ast_statement(allocator, &current, end, filename);
  if (!body) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid for statement, missing body");
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