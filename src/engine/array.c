#include "engine/array.h"

cubec_array_meta_t cubec_create_array_meta(cubec_allocator_t allocator,
                                           cubec_type_t type, size_t length) {
  cubec_array_meta_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_array_meta_t), NULL);
  self->type = type;
  self->length = length;
  return self;
}