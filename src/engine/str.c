#include "engine/str.h"
#include "core/allocator.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

static value_t str_eq(value_t self, context_t ctx, value_t another) {
  if (!value_is_comptime(self) || !value_is_comptime(another)) {
    return create_bool(ctx, false, NULL);
  }
  type_t str_t = value_get_type(self);
  type_t type = value_get_type(another);
  if (type_get_kind(type) != TYPE_KIND_STR) {
    another = value_safe_convert(another, ctx, str_t);
    if (value_is_error(another)) {
      return another;
    }
  }
  const char *str1 = *(const char **)value_get_data(self);
  const char *str2 = *(const char **)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(str1, str2) == 0, false, NULL);
}
static value_t str_ne(value_t self, context_t ctx, value_t another) {
  if (!value_is_comptime(self) || !value_is_comptime(another)) {
    return create_bool(ctx, false, NULL);
  }
  type_t str_t = value_get_type(self);
  type_t type = value_get_type(another);
  if (type_get_kind(type) != TYPE_KIND_STR) {
    another = value_safe_convert(another, ctx, str_t);
    if (value_is_error(another)) {
      return another;
    }
  }
  const char *str1 = *(const char **)value_get_data(self);
  const char *str2 = *(const char **)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(str1, str2) != 0, false, NULL);
}
static value_t str_slice(value_t self, context_t ctx, value_t start,
                         value_t end) {
  if (!value_is_comptime(self)) {
    type_t slice_type =
        create_slice_type(ctx, context_load_type(ctx, "u8"), false);
    return context_create_value(ctx, slice_type, NULL, false, false, NULL);
  }
  const char *data = *(const char **)value_get_data(self);
  type_t type = value_get_type(self);
  type_t base_type = context_load_type(ctx, "u8");
  size_t array_len = strlen(data) + 1;
  type_t start_type = value_get_type(start);
  type_t end_type = value_get_type(end);
  type_t slice_type = create_slice_type(ctx, base_type, false);
  size_t s = 0;
  size_t e = array_len;
  if (!value_is_comptime(start)) {
    return create_error(ctx, "slice start is not comptime");
  }
  if (!value_is_comptime(end)) {
    return create_error(ctx, "slice end is not comptime");
  }
  if (type_get_kind(start_type) != TYPE_KIND_VOID) {
    if (type_get_kind(start_type) == TYPE_KIND_INTEGER) {
      int64_t val = integer_get_value(start);
      if (val < 0) {
        return create_error(ctx, "slice start < 0");
      }
      s = val;
    } else if (type_get_kind(start_type) == TYPE_KIND_UNSIGNED) {
      s = unsigned_get_value(start);
    } else {
      return create_error(ctx, "slice start is not a integer");
    }
  }
  if (type_get_kind(end_type) != TYPE_KIND_VOID) {
    if (type_get_kind(end_type) == TYPE_KIND_INTEGER) {
      int64_t val = integer_get_value(end);
      if (val < 0) {
        return create_error(ctx, "slice end < 0");
      }
      e = val;
    } else if (type_get_kind(end_type) == TYPE_KIND_UNSIGNED) {
      e = unsigned_get_value(end);
    } else {
      return create_error(ctx, "slice end is not a integer");
    }
  }
  if (s >= array_len) {
    return create_error(ctx, "slice start %" PRIuPTR " >= %" PRIuPTR "", s,
                        array_len);
  }
  if (e > array_len) {
    return create_error(ctx, "slice end %" PRIuPTR " > %" PRIuPTR "", s,
                        array_len);
  }
  size_t len = 0;
  if (s < e) {
    len = e - s;
  }
  return create_comptime_slice(ctx, slice_type, (void *)data, s, len, false);
}
void str_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .type_eq = type_default_eq,
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
      .eq = str_eq,
      .ne = str_ne,
      .slice = str_slice,
  };
  type_t str_t = create_type(allocator, TYPE_KIND_STR, sizeof(const char *),
                             sizeof(const char *), "str", "str", &opt, NULL);
  context_store_type(ctx, str_t);
  create_type_value(ctx, str_t, false, "str");
}
value_t create_str(context_t ctx, const char *src) {
  type_t str_t = context_load_type(ctx, "str");
  const char *str = context_create_cstring(ctx, src);
  return context_create_value(ctx, str_t, &str, false, true, NULL);
}