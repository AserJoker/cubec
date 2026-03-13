#include "engine/scope.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/map.h"
#include "core/string.h"
#include <string.h>
#include <sys/types.h>
static void cubec_scope_dispose(cubec_scope_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->children);
  cubec_allocator_free(allocator, self->variables);
  cubec_allocator_free(allocator, self->named_variables);
  cubec_allocator_free(allocator, self->defers);
  if (self->parent) {
    cubec_list_node_t it = cubec_list_find(self->parent->children, self, NULL);
    cubec_list_erase(self->parent->children, allocator, it);
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
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->defers = cubec_create_list(allocator, &initialize);
  self->children = cubec_create_list(allocator, NULL);
  self->variables = cubec_create_list(allocator, &initialize);
  cubec_map_initialize_t map_initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->named_variables = cubec_create_map(allocator, &map_initialize);
  return self;
}
void cubec_scope_store_value(cubec_scope_t self, cubec_allocator_t allocator,
                             cubec_value_t value, const char *name) {
  cubec_list_append(self->variables, allocator, value);
  if (name) {
    if (cubec_map_has(self->named_variables, name, NULL)) {
      cubec_map_delete(self->named_variables, allocator, name, NULL);
    }
    cubec_map_set(self->named_variables, allocator,
                  cubec_create_cstring(allocator, name), value, NULL);
  }
}
cubec_value_t cubec_scope_load_value(cubec_scope_t self, const char *name) {
  return cubec_map_get(self->named_variables, name, NULL);
}