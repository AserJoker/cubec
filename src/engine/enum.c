#include "engine/enum.h"
#include "core/allocator.h"
#include <stdbool.h>
#include <string.h>

static void cubec_enum_meta_dispose(cubec_enum_meta_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->options);
}

cubec_enum_meta_t cubec_create_enum_meta(cubec_allocator_t allocator,
                                         cubec_type_t type) {
  cubec_enum_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_meta_t),
                            (cubec_dispose_fn_t)cubec_enum_meta_dispose);
  self->type = type;
  cubec_map_initialize_t initiailze = {
      .autofree_key = true,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->options = cubec_create_map(allocator, &initiailze);
  return self;
}