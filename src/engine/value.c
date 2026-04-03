#include "engine/value.h"
#include "core/allocator.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct _cubec_value_t {
  cubec_type_t type;
  bool mutable;
  void *data;
};
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool mutable, const void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  size_t size = cubec_type_get_size(type);
  self->mutable = mutable;
  self->type = type;
  if (data) {
    self->data = cubec_allocator_alloc(allocator, size, NULL);
    memcpy(self->data, data, size);
  } else {
    self->data = NULL;
  }
  return self;
}
cubec_type_t cubec_value_get_type(cubec_value_t value) { return value->type; }
bool cubec_value_is_mutable(cubec_value_t value) { return value->mutable; }
void *cubec_value_get_data(cubec_value_t value) { return value->data; }
cubec_value_t cubec_value_clone(cubec_allocator_t allocator,
                                cubec_value_t value) {
  return cubec_create_value(allocator, value->type, value->mutable,
                            value->type);
}
bool cubec_value_is_error(cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(value);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  return kind == CUBEC_VALUE_TYPE_ERROR;
}
