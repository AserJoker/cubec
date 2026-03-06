#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"

static void cubec_union_meta_dispose(cubec_union_meta_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
}

static int32_t cubec_union_type_compare(cubec_type_t a, cubec_type_t b) {
  return a->kind - b->kind;
}

cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           cubec_array_t types) {
  cubec_union_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_meta_t),
                            (cubec_dispose_fn_t)cubec_union_meta_dispose);
  self->types = types;
  return self;
}