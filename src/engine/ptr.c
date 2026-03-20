#include "engine/ptr.h"
#include "core/allocator.h"
static void cubec_ptr_meta_dispose(cubec_ptr_meta_t self,
                                   cubec_allocator_t allocator) {}
cubec_ptr_meta_t cubec_create_ptr_meta(cubec_allocator_t allocator,
                                       cubec_type_t type, bool is_mutable,
                                       bool is_volatile) {
  cubec_ptr_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ptr_meta_t),
                            (cubec_dispose_fn_t)cubec_ptr_meta_dispose);
  self->type = type;
  self->is_mutable = is_mutable;
  self->is_volatile = is_volatile;
  return self;
}