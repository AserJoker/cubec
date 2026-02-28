#include "ast/union_declarator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"
static void
cubec_ast_union_declarator_dispose(cubec_ast_union_declarator_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_union_declarator_t
cubec_create_ast_union_declarator(cubec_allocator_t allocator) {
  cubec_ast_union_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_union_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_union_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_UNION_DECLARATOR;
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->types = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_union_declarator(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_union_declarator_t node =
      cubec_create_ast_union_declarator(allocator);
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
  if (!cubec_location_is(token->loc, "union")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '<') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid union expression, missing '<'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '>') {
    for (;;) {
      cubec_ast_node_t type =
          cubec_read_ast_expression2(allocator, &current, end);
      if (!type) {
        err =
            cubec_create_ast_error(allocator, *position, current,
                                   "Invalid union expression, missing 'type'");
        goto onerror;
      }
      if (type->type == CUBEC_NODE_TYPE_ERROR) {
        err = type;
        goto onerror;
      }
      cubec_list_append(node->types, allocator, type);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '>') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid function expression");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  if (*current.offset != '>') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid function expression");
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