#include "ast/switch_case.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_switch_case(allocator_t allocator, position_t *position,
                                const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_SWITCH_CASE);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  ast_node_t statements = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "statements", statements);
  if (!token) {
    goto onerror;
  }
  if (token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (location_is(token->loc, "case")) {
    allocator_free(allocator, token);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
    ast_node_t condition =
        read_ast_expression(allocator, &current, end, filename);
    if (!condition) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid statement");
      goto onerror;
    }
    if (condition->type == NODE_TYPE_ERROR) {
      err = condition;
      goto onerror;
    }
    ast_add_child(allocator, node, "condition", condition);
  } else if (location_is(token->loc, "default")) {
    allocator_free(allocator, token);
  } else {
    allocator_free(allocator, token);
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  for (;;) {
    if (*current.offset == '}') {
      break;
    }
    ast_node_t token =
        read_ast_literal_identifier(allocator, &current, end, filename);
    if (token) {
      if (token->type == NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (location_is(token->loc, "case") ||
          location_is(token->loc, "default")) {
        current = token->loc.begin;
        allocator_free(allocator, token);
        break;
      } else {
        current = token->loc.begin;
        allocator_free(allocator, token);
      }
    }
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
    ast_node_t statement =
        read_ast_statement(allocator, &current, end, filename);
    if (!statement) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid statement");
      goto onerror;
    }
    if (statement->type == NODE_TYPE_ERROR) {
      err = statement;
      goto onerror;
    }
    ast_add_item(statements, statement);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
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