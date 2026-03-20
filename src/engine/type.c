#include "engine/type.h"
#include "core/allocator.h"

static void cubec_type_dispose(cubec_type_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->meta);
}

cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               void *meta) {
  cubec_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_type_t),
                            (cubec_dispose_fn_t)cubec_type_dispose);
  self->kind = kind;
  self->size = size;
  self->meta = meta;
  return self;
}