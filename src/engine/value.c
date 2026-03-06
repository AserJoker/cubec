#include "engine/value.h"
#include "core/allocator.h"
#include "engine/type.h"
#include <string.h>
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}

cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  self->type = type;
  self->data = data;
  return self;
}

cubec_value_t cubec_clone_value(cubec_allocator_t allocator,
                                cubec_value_t src) {
  void *data = cubec_allocator_alloc(allocator, src->type->size, NULL);
  memcpy(data, src->data, src->type->size);
  return cubec_create_value(allocator, src->type, data);
}