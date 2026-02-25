#include "engine/array.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type_kind.h"
#include "engine/value.h"

cubec_type_t cubec_create_array_type(cubec_allocator_t allocator,
                                     const char *name, cubec_type_t base_type,
                                     size_t length) {
  cubec_array_type_t type = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_array_type_t), NULL);
  type->super.kind = CUBEC_TYPE_KIND_ARRAY;
  type->super.name = name;
  type->length = length;
  return &type->super;
}

static void cubec_array_value_dispose(cubec_array_value_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}

cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       cubec_type_t type) {
  if (type->kind != CUBEC_TYPE_KIND_ARRAY) {
    return NULL;
  }
  cubec_array_type_t atype = (cubec_array_type_t)type;
  cubec_array_value_t value =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_array_value_t),
                            (cubec_dispose_fn_t)cubec_array_value_dispose);
  value->super.type = type;
  cubec_array_initialize_t initialize = {
      .autofree = false,
      .capacity = atype->length,
  };
  value->data = cubec_create_array(allocator, &initialize);
  return &value->super;
}