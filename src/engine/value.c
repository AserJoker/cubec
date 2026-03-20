#include "engine/value.h"
#include "core/allocator.h"
#include <string.h>
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool is_mutable, const void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  self->type = type;
  self->is_mutable = is_mutable;
  if (data) {
    self->data = cubec_allocator_alloc(allocator, type->size, NULL);
    memcpy(self->data, data, type->size);
  } else {
    self->data = NULL;
  }
  return self;
}
cubec_value_t cubec_create_comptime_value(cubec_allocator_t allocator,
                                          cubec_type_t type, bool is_mutable) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  self->type = type;
  self->is_mutable = is_mutable;
  self->data = NULL;
  return self;
}