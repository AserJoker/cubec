#include "ast/struct_field.h"
#include "ast/decorator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"
static void cubec_ast_struct_field_dispose(cubec_ast_struct_field_t self,
                                           cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->declarator);
  cubec_allocator_free(allocator, self->decorators);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_struct_field_t
cubec_create_ast_struct_field(cubec_allocator_t allocator) {
  cubec_ast_struct_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_struct_field_t),
                            (cubec_dispose_fn_t)cubec_ast_struct_field_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STRUCT_FIELD;
  self->declarator = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->decorators = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_struct_field(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_struct_field_t node = cubec_create_ast_struct_field(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_list_append(node->decorators, allocator, decorator);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t declarator =
      cubec_read_ast_variable_declarator(allocator, &current, end);
  if (!declarator) {
    goto onerror;
  }
  if (declarator->type == CUBEC_NODE_TYPE_ERROR) {
    err = declarator;
    goto onerror;
  }
  node->declarator = declarator;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid struct field");
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}