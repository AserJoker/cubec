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
struct _array_meta_t {
  type_t type;
  size_t length;
};
typedef struct _array_meta_t *array_meta_t;
static array_meta_t create_array_meta(allocator_t allocator, type_t type,
                                      size_t length) {
  array_meta_t self =
      allocator_alloc(allocator, sizeof(struct _array_meta_t), NULL);
  self->length = length;
  self->type = type;
  return self;
}
static bool array_type_is_equal(type_t self, type_t dst) {
  array_meta_t self_meta = type_get_meta(self);
  array_meta_t dst_meta = type_get_meta(dst);
  if (self_meta->length != dst_meta->length) {
    return false;
  }
  return type_is_equal(self_meta->type, dst_meta->type);
}
static char *array_type_to_string(type_t self, allocator_t allocator) {
  array_meta_t meta = type_get_meta(self);
  char *base_str = type_to_string(meta->type, allocator);
  size_t len = snprintf(NULL, 0, "[%" PRIuPTR "]%s", meta->length, base_str);
  char *str = allocator_alloc(allocator, len + 1, NULL);
  sprintf(str, "[%" PRIuPTR "]%s", meta->length, base_str);
  allocator_free(allocator, base_str);
  return str;
}
static value_t array_get_length(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  return create_u64(ctx, meta->length, false, NULL);
}
static value_t array_to_string(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  const char *items[meta->length];
  size_t length = 32;
  for (size_t idx = 0; idx < meta->length; idx++) {
    value_t item = value_get_index(self, ctx, idx);
    value_t str_item = value_to_string(item, ctx);
    items[idx] = *(const char **)value_get_data(str_item);
    length += strlen(items[idx]) + 1;
  }
  allocator_t allocator = context_get_allocator(ctx);
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
  value_t val = create_str(ctx, str, NULL);
  value_set_comptime(val, true);
  return val;
}
static value_t array_get_index(value_t self, context_t ctx, size_t idx) {
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  if (idx >= meta->length) {
    return create_error(
        ctx, "Array index %" PRIuPTR " is past the end of the array", idx);
  }
  uint8_t *data = value_get_data(self);
  bool mutable = value_is_mutable(self);
  if (!data) {
    return context_create_value(ctx, meta->type, mutable, NULL, NULL);
  }
  size_t offset = idx * type_get_size(meta->type);
  value_t val =
      context_create_value(ctx, meta->type, mutable, data + offset, NULL);
  value_set_comptime(val, true);
  return val;
}
static value_t array_set_index(value_t self, context_t ctx, size_t idx,
                               value_t value) {
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  type_t item_type = value_get_type(value);
  allocator_t allocator = context_get_allocator(ctx);
  if (!type_is_equal(meta->type, item_type)) {
    char *dst_type = type_to_string(meta->type, allocator);
    char *src_type = type_to_string(item_type, allocator);
    value_t error =
        create_error(ctx, "Cannot assign '%s' to '%s'", dst_type, src_type);
    allocator_free(allocator, dst_type);
    allocator_free(allocator, src_type);
    return error;
  }
  if (idx >= meta->length) {
    return create_error(
        ctx, "Array index %" PRIuPTR " is past the end of the array", idx);
  }
  if (!value_is_mutable(self)) {
    return create_error(ctx, "Cannot assign to const variable");
  }
  uint8_t *data = value_get_data(self);
  if (!data) {
    return context_get_undefined(ctx);
  }
  size_t offset = idx * type_get_size(meta->type);
  memcpy(data, value_get_data(value), type_get_size(meta->type));
  return context_get_undefined(ctx);
}
value_t create_array_type(context_t self, type_t type, size_t length) {
  array_meta_t meta =
      create_array_meta(context_get_allocator(self), type, length);
  struct _type_operator_t opt = {
      .is_type_equal = &array_type_is_equal,
      .type_to_string = &array_type_to_string,
      .to_string = &array_to_string,
      .get_length = &array_get_length,
      .get_index = &array_get_index,
  };
  return context_create_type(self, VALUE_TYPE_ARRAY,
                             length * type_get_size(type), type_get_align(type),
                             meta, &opt, NULL);
}
type_t array_type_get_type(type_t self) {
  array_meta_t meta = type_get_meta(self);
  return meta->type;
}
size_t array_type_get_length(type_t self) {
  array_meta_t meta = type_get_meta(self);
  return meta->length;
}