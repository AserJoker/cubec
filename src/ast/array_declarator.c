#include "ast/array_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_numeric.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

static void
cubec_ast_array_declarator_dispose(cubec_ast_array_declarator_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->length);
  cubec_allocator_free(allocator, self->item_type);
}

cubec_ast_array_declarator_t
cubec_create_ast_array_declarator(cubec_allocator_t allocator) {
  cubec_ast_array_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_array_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_array_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_ARRAY_DECLARATOR;
  self->length = NULL;
  self->item_type = NULL;
  return self;
}

cubec_ast_node_t cubec_read_ast_array_declarator(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_array_declarator_t node =
      cubec_create_ast_array_declarator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t length =
      cubec_read_ast_literal_numeric(allocator, &current, end);
  if (!length) {
    length = cubec_read_ast_literal_identifier(allocator, &current, end);
  }
  if (!length) {
    goto onerror;
  }
  if (length->type == CUBEC_NODE_TYPE_ERROR) {
    err = length;
    goto onerror;
  }
  if (length->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER &&
      !cubec_location_is(length->loc, "_")) {
    cubec_allocator_free(allocator, length);
    goto onerror;
  }
  node->length = length;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ']') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t item_type = cubec_read_ast_type(allocator, &current, end);
  if (!item_type) {
    goto onerror;
  }
  if (item_type->type == CUBEC_NODE_TYPE_ERROR) {
    err = item_type;
    goto onerror;
  }
  node->item_type = item_type;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}