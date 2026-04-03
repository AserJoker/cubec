#include "engine/array.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdio.h>
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
static bool cubec_array_type_is_equal(cubec_type_t self, cubec_type_t dst) {
  cubec_array_meta_t self_meta = cubec_type_get_meta(self);
  cubec_array_meta_t dst_meta = cubec_type_get_meta(dst);
  if (self_meta->length != dst_meta->length) {
    return false;
  }
  return cubec_type_is_equal(self_meta->type, dst_meta->type);
}
static char *cubec_array_type_to_string(cubec_type_t self,
                                        cubec_allocator_t allocator) {
  cubec_array_meta_t meta = cubec_type_get_meta(self);
  char *base_str = cubec_type_to_string(meta->type, allocator);
  size_t len = snprintf(NULL, 0, "[%" PRIuPTR "]%s", meta->length, base_str);
  char *str = cubec_allocator_alloc(allocator, len + 1, NULL);
  sprintf(str, "[%" PRIuPTR "]%s", meta->length, base_str);
  cubec_allocator_free(allocator, base_str);
  return str;
}
cubec_type_t cubec_create_array_type(cubec_context_t self, cubec_type_t type,
                                     size_t length) {
  cubec_array_meta_t meta =
      cubec_create_array_meta(cubec_context_get_allocator(self), type, length);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = &cubec_array_type_is_equal,
      .type_to_string = &cubec_array_type_to_string,
  };
  return cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_ARRAY, length * cubec_type_get_size(type),
      cubec_type_get_align(type), meta, &opt, NULL);
}
cubec_type_t cubec_array_type_get_type(cubec_type_t self) {
  cubec_array_meta_t meta = cubec_type_get_meta(self);
  return meta->type;
}
size_t cubec_array_type_get_length(cubec_type_t self) {
  cubec_array_meta_t meta = cubec_type_get_meta(self);
  return meta->length;
}