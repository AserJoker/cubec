#include "ast/switch_case.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_switch_case(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end,
                                            const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_SWITCH_CASE);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  cubec_ast_node_t statements =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "statements", statements);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (cubec_location_is(token->loc, "case")) {
    cubec_allocator_free(allocator, token);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t condition =
        cubec_read_ast_expression(allocator, &current, end, filename);
    if (!condition) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "invalid statement");
      goto onerror;
    }
    if (condition->type == CUBEC_NODE_TYPE_ERROR) {
      err = condition;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "condition", condition);
  } else if (cubec_location_is(token->loc, "default")) {
    cubec_allocator_free(allocator, token);
  } else {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid statement");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  for (;;) {
    if (*current.offset == '}') {
      break;
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end, filename);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (cubec_location_is(token->loc, "case") ||
          cubec_location_is(token->loc, "default")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
        break;
      } else {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
      }
    }
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t statement =
        cubec_read_ast_statement(allocator, &current, end, filename);
    if (!statement) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "invalid statement");
      goto onerror;
    }
    if (statement->type == CUBEC_NODE_TYPE_ERROR) {
      err = statement;
      goto onerror;
    }
    cubec_ast_add_item(statements, statement);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}