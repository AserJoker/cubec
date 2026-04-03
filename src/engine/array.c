#include "engine/array.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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
static cubec_value_t cubec_array_get_length(cubec_value_t self,
                                            cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_array_meta_t meta = cubec_type_get_meta(type);
  return cubec_create_uint64(ctx, meta->length, false, NULL);
}
static cubec_value_t cubec_array_to_string(cubec_value_t self,
                                           cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_array_meta_t meta = cubec_type_get_meta(type);
  const char *items[meta->length];
  size_t length = 32;
  for (size_t idx = 0; idx < meta->length; idx++) {
    cubec_value_t item = cubec_value_get_index(self, ctx, idx);
    cubec_value_t str_item = cubec_value_to_string(item, ctx);
    items[idx] = *(const char **)cubec_value_get_data(str_item);
    length += strlen(items[idx]) + 1;
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char str[length];
  size_t offset = 0;
  str[offset++] = '[';
  for (size_t idx = 0; idx < meta->length; idx++) {
    if (idx != 0) {
      str[offset++] = ',';
      str[offset++] = ' ';
    }
    strcpy(&str[offset], items[idx]);
  }
  str[offset++] = ']';
  str[offset] = 0;
  return cubec_create_str(ctx, str, NULL);
}
static cubec_value_t cubec_array_get_index(cubec_value_t self,
                                           cubec_context_t ctx, size_t idx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_array_meta_t meta = cubec_type_get_meta(type);
  if (idx >= meta->length) {
    return cubec_create_error(
        ctx, "Array index %" PRIuPTR " is past the end of the array", idx);
  }
  size_t offset = idx * cubec_type_get_size(meta->type);
  uint8_t *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  return cubec_context_create_value(ctx, meta->type, mutable, data + offset,
                                    NULL);
}
static cubec_value_t cubec_array_set_index(cubec_value_t self,
                                           cubec_context_t ctx, size_t idx,
                                           cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_array_meta_t meta = cubec_type_get_meta(type);
  cubec_type_t item_type = cubec_value_get_type(value);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!cubec_type_is_equal(meta->type, item_type)) {
    char *dst_type = cubec_type_to_string(meta->type, allocator);
    char *src_type = cubec_type_to_string(item_type, allocator);
    cubec_value_t error = cubec_create_error(ctx, "Cannot assign '%s' to '%s'",
                                             dst_type, src_type);
    cubec_allocator_free(allocator, dst_type);
    cubec_allocator_free(allocator, src_type);
    return error;
  }
  if (idx >= meta->length) {
    return cubec_create_error(
        ctx, "Array index %" PRIuPTR " is past the end of the array", idx);
  }
  if (!cubec_value_is_mutable(self)) {
    return cubec_create_error(ctx, "Cannot assign to const variable");
  }
  size_t offset = idx * cubec_type_get_size(meta->type);
  uint8_t *data = cubec_value_get_data(self);
  memcpy(data, cubec_value_get_data(value), cubec_type_get_size(meta->type));
  return cubec_context_get_undefined(ctx);
}
cubec_type_t cubec_create_array_type(cubec_context_t self, cubec_type_t type,
                                     size_t length) {
  cubec_array_meta_t meta =
      cubec_create_array_meta(cubec_context_get_allocator(self), type, length);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = &cubec_array_type_is_equal,
      .type_to_string = &cubec_array_type_to_string,
      .to_string = &cubec_array_to_string,
      .get_length = &cubec_array_get_length,
      .get_index = &cubec_array_get_index,
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