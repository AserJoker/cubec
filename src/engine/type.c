#include "engine/type.h"
#include "core/allocator.h"
struct _cubec_type_t {
  cubec_type_kind_t kind;
  size_t size;
  void *meta;
};
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
  self->meta = meta;
  self->size = size;
  return self;
}
cubec_type_kind_t cubec_type_get_kind(cubec_type_t self) { return self->kind; }
size_t cubec_type_get_size(cubec_type_t self) { return self->size; }
void cubec_type_set_size(cubec_type_t self, size_t size) { self->size = size; }
void *cubec_type_get_meta(cubec_type_t self) { return self->meta; }