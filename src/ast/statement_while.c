#include "ast/statement_while.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_while(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_WHILE);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "while")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing '('");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t condition =
      cubec_read_ast_expression(allocator, &current, end);
  if (!condition) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing condition");
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "condition", condition);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid if statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body = cubec_read_ast_statement(allocator, &current, end);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid while statement, missing body");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "body", body);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}