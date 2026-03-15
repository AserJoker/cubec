#include "ast/statement_test.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_statement_test_dispose(cubec_ast_statement_test_t self,
                                             cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->body);
  cubec_allocator_free(allocator, self->name);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_test_t
cubec_create_ast_statement_test(cubec_allocator_t allocator) {
  cubec_ast_statement_test_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_test_t),
      (cubec_dispose_fn_t)cubec_ast_statement_test_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_TEST;
  self->name = NULL;
  self->body = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_test(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end) {
  cubec_ast_statement_test_t node = cubec_create_ast_statement_test(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "test")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t name =
      cubec_read_ast_literal_string(allocator, &current, end);
  if (!name) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid test statement, missing name");
    goto onerror;
  }
  if (name->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  node->name = name;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t body =
      cubec_read_ast_statement_block(allocator, &current, end);
  if (!body) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid test statement, missing body");
    goto onerror;
  }
  if (body->type == CUBEC_NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  node->body = body;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}