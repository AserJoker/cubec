#include "engine/array.h"
#include "core/allocator.h"

static void cubec_array_meta_dispose(cubec_array_meta_t self,
                                     cubec_allocator_t allocator) {}

cubec_array_meta_t cubec_create_array_meta(cubec_allocator_t allocator,
                                           cubec_type_t type, size_t length) {
  cubec_array_meta_t meta =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_array_meta_t),
                            (cubec_dispose_fn_t)cubec_array_meta_dispose);
  meta->type = type;
  meta->len = length;
  return meta;
}