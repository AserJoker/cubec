#include "ast/statement_return.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_statement_return_dispose(cubec_ast_statement_return_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_return_t
cubec_create_ast_statement_return(cubec_allocator_t allocator) {
  cubec_ast_statement_return_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_return_t),
      (cubec_dispose_fn_t)cubec_ast_statement_return_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_RETURN;
  self->value = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_return(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_statement_return_t node =
      cubec_create_ast_statement_return(allocator);
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
  if (!cubec_location_is(token->loc, "return")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t value = cubec_read_ast_expression(allocator, &current, end);
  if (value) {
    if (value->type == CUBEC_NODE_TYPE_ERROR) {
      err = value;
      goto onerror;
    }
    node->value = value;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing ';'");
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