#include "engine/result.h"
#include "core/allocator.h"

cubec_result_meta_t cubec_create_result_meta(cubec_allocator_t allocator,
                                             cubec_type_t type,
                                             cubec_type_t etype) {
  cubec_result_meta_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_result_meta_t), NULL);
  self->type = type;
  self->error_type = etype;
  return self;
}