#include "ast/statement_continue.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
static void
cubec_ast_statement_continue_dispose(cubec_ast_statement_continue_t self,
                                     cubec_allocator_t allocator) {
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_continue_t
cubec_create_ast_statement_continue(cubec_allocator_t allocator) {
  cubec_ast_statement_continue_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_continue_t),
      (cubec_dispose_fn_t)cubec_ast_statement_continue_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_CONTINUE;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_continue(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end) {
  cubec_ast_statement_continue_t node =
      cubec_create_ast_statement_continue(allocator);
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
  if (!cubec_location_is(token->loc, "continue")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
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