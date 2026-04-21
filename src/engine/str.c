#include "engine/str.h"
#include "core/allocator.h"
#include "engine/bool.h"
#include "engine/context.h"
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
void str_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
      .eq = str_eq,
      .ne = str_ne,
  };
  type_t str_t = create_type(allocator, TYPE_KIND_STR, sizeof(const char *),
                             sizeof(const char *), "str", "str", &opt, NULL);
  context_store_type(ctx, str_t);
}
value_t create_str(context_t ctx, const char *src) {
  type_t str_t = context_load_type(ctx, "str");
  const char *str = context_create_cstring(ctx, src);
  return context_create_value(ctx, str_t, &str, false, true, NULL);
}