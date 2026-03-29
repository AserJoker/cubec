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

cubec_ast_node_t cubec_read_ast_statement_for(cubec_allocator_t allocator,
                                              cubec_position_t *position,
                                              const char *end,
                                              const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_FOR);
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
  if (!cubec_location_is(token->loc, "for")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
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
  cubec_ast_node_t init =
      cubec_read_ast_statement_declaration(allocator, &current, end, filename);
  if (!init) {
    init =
        cubec_read_ast_statement_expression(allocator, &current, end, filename);
  }
  if (!init) {
    init = cubec_read_ast_statement_empty(allocator, &current, end, filename);
  }
  if (!init) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid for statement, missing initialize");
    goto onerror;
  }
  if (init->type == CUBEC_NODE_TYPE_ERROR) {
    err = init;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "init", init);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t conditon =
      cubec_read_ast_statement_expression(allocator, &current, end, filename);
  if (!conditon) {
    conditon =
        cubec_read_ast_statement_empty(allocator, &current, end, filename);
  }
  if (!conditon) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid for statement, missing condition");
    goto onerror;
  }
  if (conditon->type == CUBEC_NODE_TYPE_ERROR) {
    err = conditon;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "condition", conditon);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t after =
      cubec_read_ast_expression(allocator, &current, end, filename);
  if (after) {
    if (after->type == CUBEC_NODE_TYPE_ERROR) {
      err = after;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "after", after);
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
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
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid for statement, missing body");
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