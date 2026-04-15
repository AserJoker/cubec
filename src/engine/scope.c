#include "engine/scope.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/string.h"
#include <string.h>
struct _scope_t {
  scope_t parent;
  array_t values;
  hash_map_t variables;
};
static void scope_dispose(scope_t self, allocator_t allocator) {
  allocator_free(allocator, self->variables);
  allocator_free(allocator, self->values);
}
scope_t create_scope(allocator_t allocator, scope_t parent) {
  scope_t self = allocator_alloc(allocator, sizeof(struct _scope_t),
                                 (dispose_fn_t)scope_dispose);
  self->parent = parent;
  array_initialize_t array_initialize = {
      .autofree = true,
  };
  self->values = create_array(allocator, &array_initialize);
  hash_map_initialize_t map_initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
  };
  self->variables = create_hash_map(allocator, &map_initialize);
  return self;
}
void scope_store(scope_t self, allocator_t allocator, value_t value,
                 const char *name) {
  array_push(self->values, value);
  if (name) {
    hash_map_set(self->variables, create_cstring(allocator, name), value, NULL,
                 NULL);
  }
}
value_t scope_load(scope_t self, const char *name) {
  return hash_map_get(self->variables, name, NULL, NULL);
}
scope_t scope_get_parent(scope_t scope) { return scope->parent; }