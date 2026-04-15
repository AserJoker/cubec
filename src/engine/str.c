#include "engine/str.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static char *str_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "str");
}
static value_t str_get_length(value_t self, context_t ctx) {
  const char *data = *(const char **)value_get_data(self);
  return create_u64(ctx, strlen(data), false, NULL);
}
static value_t str_get_index(value_t self, context_t ctx, size_t idx) {
  const char *data = *(const char **)value_get_data(self);
  return create_i8(ctx, data[idx], false, NULL);
}
static value_t str_to_string(value_t self, context_t ctx) {
  const char *data = *(const char **)value_get_data(self);
  size_t len = strlen(data) + 3;
  char str[len];
  sprintf(str, "\"%s\"", data);
  return create_str(ctx, str, NULL);
}
void init_str_type(context_t ctx) {
  struct _type_operator_t opt = {
      .type_to_string = &str_type_to_string,
      .get_length = &str_get_length,
      .get_index = &str_get_index,
      .to_string = &str_to_string,
  };
  context_create_type(ctx, CUBEC_VALUE_TYPE_STR, sizeof(const char **),
                      sizeof(const char **), NULL, &opt, "str");
}
value_t create_str(context_t ctx, const char *data, const char *name) {
  value_t vtype = context_load(ctx, "str");
  type_t type = *(type_t *)value_get_data(vtype);
  const char *str = context_create_cstring(ctx, data);
  return context_create_value(ctx, type, false, &str, name);
}