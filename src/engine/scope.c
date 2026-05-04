#include "engine/scope.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/string.h"
#include <string.h>
struct _scope_t {
  scope_t parent;
  list_t children;
  hash_map_t variables;
  array_t values;
  bool is_function;
};
static void scope_dispose(scope_t self, allocator_t allocator) {
  if (self->parent) {
    list_node_t it = list_find(self->parent->children, self, NULL);
    list_erase(self->parent->children, it);
    self->parent = NULL;
  }
  while (list_get_size(self->children)) {
    list_node_t it = list_get_first(self->children);
    scope_t child = list_node_get(it);
    allocator_free(allocator, child);
  }
  allocator_free(allocator, self->children);
  allocator_free(allocator, self->values);
  allocator_free(allocator, self->variables);
}
scope_t create_scope(allocator_t allocator, scope_t parent) {
  scope_t self = allocator_alloc(allocator, sizeof(struct _scope_t),
                                 (dispose_fn_t)scope_dispose);
  self->children = create_list(allocator, NULL);
  array_initialize_t values_initialize = {
      .autofree = true,
  };
  self->values = create_array(allocator, &values_initialize);
  hash_map_initialize_t hash_initialize = {
      .hash = (hash_fn_t)cstring_sdb,
      .autofree_key = true,
      .autofree_value = false,
      .compare = (compare_fn_t)strcmp,
  };
  self->variables = create_hash_map(allocator, &hash_initialize);
  self->parent = parent;
  if (parent) {
    list_append(parent->children, self);
  }
  self->is_function = false;
  return self;
}
value_t scope_load(scope_t self, const char *name) {
  return hash_map_get(self->variables, name, NULL, NULL);
}
void scope_store(scope_t self, allocator_t allocator, const char *name,
                 value_t value) {
  array_push(self->values, value);
  if (name) {
    hash_map_set(self->variables, create_cstring(allocator, name), value, NULL,
                 NULL);
  }
}
scope_t scope_get_parent(scope_t self) { return self->parent; }
void scope_set_is_function(scope_t scope, bool is_function) {
  scope->is_function = is_function;
}
bool scope_is_function(scope_t scope) { return scope->is_function; }