#include "ast/statement_switch.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/switch_case.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_switch(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_SWITCH);
  cubec_ast_node_t cases =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "cases", cases);
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
  if (!cubec_location_is(token->loc, "switch")) {
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
                                 "Invalid switch statement, missing '('");
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
                                 "Invalid switch statement, missing condition");
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
                                 "Invalid switch statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid switch statement, missing '{'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      cubec_ast_node_t cas =
          cubec_read_ast_switch_case(allocator, &current, end);
      if (!cas) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid switch statement");
        goto onerror;
      }
      if (cas->type == CUBEC_NODE_TYPE_ERROR) {
        err = cas;
        goto onerror;
      }
      cubec_ast_add_item(allocator, cases, cas);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
    }
  }
  if (*current.offset != '}') {
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  current.offset++;
  current.column++;

  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}