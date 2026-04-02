#include "engine/array.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
struct _cubec_array_meta_t {
  cubec_type_t type;
  size_t length;
};
typedef struct _cubec_array_meta_t *cubec_array_meta_t;
static cubec_array_meta_t cubec_create_array_meta(cubec_allocator_t allocator,
                                                  cubec_type_t type,
                                                  size_t length) {
  cubec_array_meta_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_array_meta_t), NULL);
  self->length = length;
  self->type = type;
  return self;
}
cubec_type_t cubec_create_array_type(cubec_context_t self, cubec_type_t type,
                                     size_t length) {
  cubec_array_meta_t meta =
      cubec_create_array_meta(cubec_context_get_allocator(self), type, length);
  return cubec_context_create_type(self, CUBEC_VALUE_TYPE_ARRAY,
                                   length * cubec_type_get_size(type),
                                   cubec_type_get_align(type), meta);
}
cubec_type_t cubec_array_type_get_type(cubec_type_t self) {
  cubec_array_meta_t meta = cubec_type_get_meta(self);
  return meta->type;
}
size_t cubec_array_type_get_length(cubec_type_t self) {
  cubec_array_meta_t meta = cubec_type_get_meta(self);
  return meta->length;
}