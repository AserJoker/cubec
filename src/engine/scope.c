#include "engine/scope.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
static void cubec_scope_dispose(cubec_scope_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->defers);
  cubec_allocator_free(allocator, self->values);
  cubec_allocator_free(allocator, self->variables);
}
cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent) {
  cubec_scope_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_scope_t),
                            (cubec_dispose_fn_t)cubec_scope_dispose);
  self->parent = parent;
  self->defers = cubec_create_array(allocator, NULL);
  cubec_array_initialize_t values_initialize = {
      .autofree = true,
  };
  self->values = cubec_create_array(allocator, &values_initialize);
  cubec_map_initialize_t variables_initialize = {
      .autofree_key = true,
      .autofree_value = false,
  };
  self->variables = cubec_create_map(allocator, &variables_initialize);
  return self;
}