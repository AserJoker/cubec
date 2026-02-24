#include "ast/statement_switch.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/switch_case.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_statement_switch_dispose(cubec_ast_statement_switch_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->condition);
  cubec_allocator_free(allocator, self->cases);
}
cubec_ast_statement_switch_t
cubec_create_ast_statement_switch(cubec_allocator_t allocator) {
  cubec_ast_statement_switch_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_switch_t),
      (cubec_dispose_fn_t)cubec_ast_statement_switch_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_SWITCH;
  self->condition = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->cases = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_switch(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_statement_switch_t node =
      cubec_create_ast_statement_switch(allocator);
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
  node->condition = condition;
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
      cubec_list_append(node->cases, allocator, cas);
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

  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}