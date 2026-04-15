#include "ast/statement_if.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_if(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_IF);
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
  if (!location_is(token->loc, "if")) {
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
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t condition =
      read_ast_expression(allocator, &current, end, filename);
  if (!condition) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing condition");
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  ast_add_child(allocator, node, "condition", condition);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid if statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t consequent =
      read_ast_statement(allocator, &current, end, filename);
  if (!consequent) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing consequent");
    goto onerror;
  }
  if (consequent->type == CUBEC_NODE_TYPE_ERROR) {
    err = consequent;
    goto onerror;
  }
  ast_add_child(allocator, node, "consequent", consequent);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  token = read_ast_literal_identifier(allocator, &current, end, filename);
  if (token && token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (token && location_is(token->loc, "else")) {
    allocator_free(allocator, token);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    ast_node_t alternate =
        read_ast_statement(allocator, &current, end, filename);
    if (!alternate) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid statement, missing alternate");
      goto onerror;
    }
    if (alternate->type == CUBEC_NODE_TYPE_ERROR) {
      err = alternate;
      goto onerror;
    }
    ast_add_child(allocator, node, "alternate", alternate);
  } else {
    allocator_free(allocator, token);
    current = consequent->loc.end;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}