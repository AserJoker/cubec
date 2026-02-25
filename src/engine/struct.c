#include "engine/struct.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/map.h"
#include "engine/type_kind.h"
#include <string.h>
static void cubec_struct_type_dispose(cubec_struct_type_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->attributes);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->methods);
}
cubec_type_t cubec_create_struct_type(cubec_allocator_t allocator,
                                      const char *name) {
  cubec_struct_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_type_t),
                            (cubec_dispose_fn_t)cubec_struct_type_dispose);
  self->super.kind = CUBEC_TYPE_KIND_STRUCT;
  self->super.name = name;
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->methods = cubec_create_map(allocator, &initialize);
  self->fields = cubec_create_map(allocator, &initialize);
  self->attributes = cubec_create_map(allocator, &initialize);
  return &self->super;
}
static void cubec_struct_value_dispose(cubec_struct_value_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
}
cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        cubec_type_t type) {
  cubec_struct_value_t value =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_value_t),
                            (cubec_dispose_fn_t)cubec_struct_value_dispose);
  value->super.type = type;
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  value->fields = cubec_create_map(allocator, &initialize);
  return &value->super;
}