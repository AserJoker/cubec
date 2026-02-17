#include "ast/array_declarator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"
static void
cubec_ast_array_declarator_dispose(cubec_ast_array_declarator_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->items);
}
cubec_ast_array_declarator_t
cubec_create_ast_array_declarator(cubec_allocator_t allocator) {
  cubec_ast_array_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_array_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_array_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_ARRAY_DECLARATOR;
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->items = cubec_create_list(allocator, &initialize);
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
  if (*current.offset != ']') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_expression3(allocator, &current, end);
      if (!item) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      cubec_list_append(node->items, allocator, item);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ']') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ']') {
        break;
      }
    }
  }
  if (*current.offset != ']') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
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