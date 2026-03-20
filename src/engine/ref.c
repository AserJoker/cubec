#include "engine/ref.h"
#include "core/allocator.h"
cubec_ref_meta_t cubec_create_ref_meta(cubec_allocator_t allocator,
                                       cubec_type_t type) {
  cubec_ref_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_meta_t), NULL);
  self->type = type;
  return self;
}