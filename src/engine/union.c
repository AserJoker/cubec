#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
static void cubec_union_meta_dispose(cubec_union_meta_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
}
cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           size_t size, cubec_type_t *types) {
  cubec_union_meta_t meta = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_union_meta_t), NULL);
  meta->types = cubec_create_array(allocator, NULL);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_array_push(meta->types, allocator, types[idx]);
  }
  return meta;
}