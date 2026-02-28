#include "engine/scope.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/list.h"
#include "core/map.h"
#include <string.h>

static void cubec_scope_dispose(cubec_scope_t self,
                                cubec_allocator_t allocator) {
  while (cubec_list_get_size(self->children)) {
    cubec_list_node_t it = cubec_list_get_first(self->children);
    cubec_scope_t scope = cubec_list_node_get(it);
    cubec_allocator_free(allocator, scope);
  }
  cubec_allocator_free(allocator, self->children);
  cubec_allocator_free(allocator, self->variables);
  cubec_allocator_free(allocator, self->named_variables);
  if (self->parent) {
    cubec_list_node_t it = cubec_list_find(self->parent->children, self, NULL);
    if (it != cubec_list_get_end(self->parent->children)) {
      cubec_list_set_data(self->parent->children, allocator, it, NULL);
      cubec_list_erase(self->parent->children, allocator, it);
    }
  }
}

cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent) {
  cubec_scope_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_scope_t),
                            (cubec_dispose_fn_t)cubec_scope_dispose);
  self->parent = parent;
  if (parent) {
    cubec_list_append(parent->children, allocator, self);
  }
  self->defers = cubec_create_list(allocator, NULL);
  self->children = cubec_create_list(allocator, NULL);
  cubec_list_initialize_t l_initialize = {
      .autofree = true,
  };
  self->variables = cubec_create_list(allocator, &l_initialize);
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->named_variables = cubec_create_map(allocator, &initialize);
  return self;
}