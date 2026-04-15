#include "ast/statement_switch.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/switch_case.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_switch(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_SWITCH);
  ast_node_t cases = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "cases", cases);
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
  if (!location_is(token->loc, "switch")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '(') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid switch statement, missing '('");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t condition =
      read_ast_expression(allocator, &current, end, filename);
  if (!condition) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid switch statement, missing condition");
    goto onerror;
  }
  if (condition->type == NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  ast_add_child(allocator, node, "condition", condition);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid switch statement, missing ')'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid switch statement, missing '{'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      ast_node_t cas = read_ast_switch_case(allocator, &current, end, filename);
      if (!cas) {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid switch statement");
        goto onerror;
      }
      if (cas->type == NODE_TYPE_ERROR) {
        err = cas;
        goto onerror;
      }
      ast_add_item(cases, cas);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
    }
  }
  if (*current.offset != '}') {
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
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