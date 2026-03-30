#include "engine/scope.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include <string.h>
struct _cubec_scope_t {
  cubec_scope_t parent;
  cubec_array_t values;
  cubec_map_t variables;
};
static void cubec_scope_dispose(cubec_scope_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->variables);
  cubec_allocator_free(allocator, self->values);
}
cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent) {
  cubec_scope_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_scope_t),
                            (cubec_dispose_fn_t)cubec_scope_dispose);
  self->parent = parent;
  cubec_array_initialize_t array_initialize = {
      .autofree = true,
  };
  self->values = cubec_create_array(allocator, &array_initialize);
  cubec_map_initialize_t map_initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->variables = cubec_create_map(allocator, &map_initialize);
  return self;
}
void cubec_scope_store(cubec_scope_t self, cubec_allocator_t allocator,
                       cubec_value_t value, const char *name) {
  cubec_array_push(self->values, value);
  if (name) {
    cubec_map_set(self->variables, cubec_create_cstring(allocator, name), value,
                  NULL);
  }
}
cubec_value_t cubec_scope_load(cubec_scope_t self, const char *name) {
  return cubec_map_get(self->variables, name, NULL);
}
cubec_scope_t cubec_scope_get_parent(cubec_scope_t scope) {
  return scope->parent;
}