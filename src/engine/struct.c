#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include <string.h>

static void cubec_struct_field_desc_dispose(cubec_struct_field_desc_t self,
                                            cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}

cubec_struct_field_desc_t
cubec_create_struct_field_desc(cubec_allocator_t allocator, const char *name,
                               size_t offset, cubec_type_t type) {
  cubec_struct_field_desc_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_struct_field_desc_t),
      (cubec_dispose_fn_t)cubec_struct_field_desc_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->offset = offset;
  self->type = type;
  return self;
}

static void cubec_struct_meta_dispose(cubec_struct_meta_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
}

cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator) {
  cubec_struct_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_meta_t),
                            (cubec_dispose_fn_t)cubec_struct_meta_dispose);
  cubec_map_initialize_t attribute_initialize = {
      .autofree_key = true,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->attributes = cubec_create_map(allocator, &attribute_initialize);
  cubec_array_initialize_t field_initialize = {
      .autofree = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->fields = cubec_create_array(allocator, &field_initialize);
  self->align = 0;
  return self;
}