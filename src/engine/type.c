#include "engine/type.h"
#include "core/allocator.h"
#include "core/string.h"
static void cubec_type_dispose(cubec_type_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->meta);
  cubec_allocator_free(allocator, self->name);
}
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               const char *name, void *meta) {
  cubec_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_type_t),
                            (cubec_dispose_fn_t)cubec_type_dispose);
  self->kind = kind;
  self->size = size;
  if (name) {
    self->name = cubec_create_cstring(allocator, name);
  } else {
    self->name = NULL;
  }
  self->meta = meta;
  return self;
}