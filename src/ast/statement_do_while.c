#include "ast/statement_do_while.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_statement_do_while_dispose(cubec_ast_statement_do_while_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->condition);
  cubec_allocator_free(allocator, self->body);
}
cubec_ast_statement_do_while_t
cubec_create_ast_statement_do_while(cubec_allocator_t allocator) {
  cubec_ast_statement_do_while_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_do_while_t),
      (cubec_dispose_fn_t)cubec_ast_statement_do_while_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_DO_WHILE;
  self->condition = NULL;
  self->body = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_do_while(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end) {
  cubec_ast_statement_do_while_t node =
      cubec_create_ast_statement_do_while(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;

  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "do")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body = cubec_read_ast_statement(allocator, &current, end);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  node->body = body;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  token = cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "while")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement, missing '('");
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
                                 "Invalid do-while statement");
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  node->condition = condition;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid do-while statement, missing ';'");
    goto onerror;
  }
  current.offset++;
  current.column++;

  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
