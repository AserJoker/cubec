#include "engine/str.h"
#include "core/allocator.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>

static value_t str_eq(value_t self, context_t ctx, value_t another) {
  type_t str_t = value_get_type(self);
  type_t type = value_get_type(another);
  if (type_get_kind(type) != TYPE_KIND_STR) {
    another = value_safe_convert(another, ctx, str_t);
    type = value_get_type(another);
    if (type_get_kind(type) == TYPE_KIND_ERROR) {
      return another;
    }
  }
  const char *str1 = *(const char **)value_get_data(self);
  const char *str2 = *(const char **)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(str1, str2) == 0, false, NULL);
}
static value_t str_ne(value_t self, context_t ctx, value_t another) {
  type_t str_t = value_get_type(self);
  type_t type = value_get_type(another);
  if (type_get_kind(type) != TYPE_KIND_STR) {
    another = value_safe_convert(another, ctx, str_t);
    type = value_get_type(another);
    if (type_get_kind(type) == TYPE_KIND_ERROR) {
      return another;
    }
  }
  const char *str1 = *(const char **)value_get_data(self);
  const char *str2 = *(const char **)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(str1, str2) != 0, false, NULL);
}
static value_t str_safe_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_PARRAY) {
    type_t base_type = ptr_type_get_type(type);
    if (strcmp(type_get_id(base_type), "u8") == 0) {
      const char *data = *(const char **)value_get_data(self);
      return context_create_value(ctx, type, &data, false, true, NULL);
    }
  }
  return create_error(ctx, "cannot convert 'str' to '%s'", type_get_name(type));
}
void str_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .type_eq = type_default_eq,
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
      .eq = str_eq,
      .ne = str_ne,
      .safe_convert = str_safe_convert,
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