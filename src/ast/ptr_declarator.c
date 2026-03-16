#include "ast/ptr_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_ptr_declarator_dispose(cubec_ast_ptr_declarator_t self,
                                             cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->kind);
  cubec_allocator_free(allocator, self->type);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_ptr_declarator_t
cubec_create_ast_ptr_declarator(cubec_allocator_t allocator) {
  cubec_ast_ptr_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_ptr_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_ptr_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_PTR_DECLARATOR;
  cubec_ast_set_field(self, allocator, decorators);
  cubec_ast_set_field(self, allocator, type);
  cubec_ast_set_field(self, allocator, kind);
  self->decorators = cubec_create_ast_list_node(allocator);
  return self;
}
cubec_ast_node_t cubec_read_ast_ptr_declarator(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end) {
  cubec_ast_ptr_declarator_t node = cubec_create_ast_ptr_declarator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t kind =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == CUBEC_NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!cubec_location_is(kind->loc, "&") &&
      !cubec_location_is(kind->loc, "*") &&
      !cubec_location_is(kind->loc, "[*]")) {
    cubec_allocator_free(allocator, kind);
    goto onerror;
  }
  node->kind = kind;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  for (;;) {
    cubec_ast_node_t item =
        cubec_read_ast_literal_identifier(allocator, &current, end);
    if (!item) {
      break;
    }
    if (item->type == CUBEC_NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    if (cubec_location_is(item->loc, "volatile") ||
        cubec_location_is(item->loc, "const")) {
      cubec_ast_list_node_append(node->decorators, allocator, item);
    } else {
      current = item->loc.begin;
      cubec_allocator_free(allocator, item);
      break;
    }
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t type = cubec_read_ast_type(allocator, &current, end);
  if (!type) {
    goto onerror;
  }
  if (type->type == CUBEC_NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  node->type = type;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->decorators, &node->super);
  cubec_ast_set_parent(node->kind, &node->super);
  cubec_ast_set_parent(node->type, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}