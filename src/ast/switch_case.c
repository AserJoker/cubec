#include "ast/switch_case.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_switch_case_dispose(cubec_ast_switch_case_t self,
                                          cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->condition);
  cubec_allocator_free(allocator, self->statements);
}
cubec_ast_switch_case_t
cubec_create_ast_switch_case(cubec_allocator_t allocator) {
  cubec_ast_switch_case_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_switch_case_t),
                            (cubec_dispose_fn_t)cubec_ast_switch_case_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_SWITCH_CASE;
  self->condition = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->statements = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_switch_case(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_switch_case_t node = cubec_create_ast_switch_case(allocator);
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
  if (cubec_location_is(token->loc, "case")) {
    cubec_allocator_free(allocator, token);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t condition =
        cubec_read_ast_expression(allocator, &current, end);
    if (!condition) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid statement");
      goto onerror;
    }
    if (condition->type == CUBEC_NODE_TYPE_ERROR) {
      err = condition;
      goto onerror;
    }
    node->condition = condition;
  } else if (cubec_location_is(token->loc, "default")) {
    cubec_allocator_free(allocator, token);
  } else {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  for (;;) {
    if (*current.offset == '}') {
      break;
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end);
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
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t statement =
        cubec_read_ast_statement(allocator, &current, end);
    if (!statement) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid statement");
      goto onerror;
    }
    if (statement->type == CUBEC_NODE_TYPE_ERROR) {
      err = statement;
      goto onerror;
    }
    cubec_list_append(node->statements, allocator, statement);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}