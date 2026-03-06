#include "engine/ptr.h"
#include "core/allocator.h"

cubec_ptr_meta_t cubec_create_ptr_meta(cubec_allocator_t allocator,
                                       cubec_type_t type) {
  cubec_ptr_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ptr_meta_t), NULL);
  self->type = type;
  return self;
}
cubec_ptr_array_meta_t cubec_create_ptr_array_meta(cubec_allocator_t allocator,
                                                   cubec_type_t type) {
  cubec_ptr_array_meta_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ptr_array_meta_t), NULL);
  self->type = type;
  return self;
}