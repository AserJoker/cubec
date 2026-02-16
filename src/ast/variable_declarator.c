#include "ast/variable_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_variable_declarator_dispose(cubec_ast_variable_declarator_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->type);
  cubec_allocator_free(allocator, self->initialize);
}
cubec_ast_variable_declarator_t
cubec_create_ast_variable_declarator(cubec_allocator_t allocator) {
  cubec_ast_variable_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_variable_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_variable_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_VARIABLE_DECLARATOR;
  self->identifier = NULL;
  self->type = NULL;
  self->initialize = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_variable_declarator(cubec_allocator_t allocator,
                                                    cubec_position_t *position,
                                                    const char *end) {
  cubec_ast_variable_declarator_t node =
      cubec_create_ast_variable_declarator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  node->identifier = identifier;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset == ':') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_node_t type =
        cubec_read_ast_expression3(allocator, &current, end);
    if (!type) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (type->type == CUBEC_NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    node->type = type;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset == '=') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_node_t initialize =
        cubec_read_ast_expression2(allocator, &current, end);
    if (!initialize) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (initialize->type == CUBEC_NODE_TYPE_ERROR) {
      err = initialize;
      goto onerror;
    }
    node->initialize = initialize;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}