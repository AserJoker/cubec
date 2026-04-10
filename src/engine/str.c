#include "engine/str.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static char *cubec_str_type_to_string(cubec_type_t self,
                                      cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "str");
}
static cubec_value_t cubec_str_get_length(cubec_value_t self,
                                          cubec_context_t ctx) {
  const char *data = *(const char **)cubec_value_get_data(self);
  return cubec_create_u64(ctx, strlen(data), false, NULL);
}
static cubec_value_t cubec_str_get_index(cubec_value_t self,
                                         cubec_context_t ctx, size_t idx) {
  const char *data = *(const char **)cubec_value_get_data(self);
  return cubec_create_i8(ctx, data[idx], false, NULL);
}
static cubec_value_t cubec_str_to_string(cubec_value_t self,
                                         cubec_context_t ctx) {
  const char *data = *(const char **)cubec_value_get_data(self);
  size_t len = strlen(data) + 3;
  char str[len];
  sprintf(str, "\"%s\"", data);
  return cubec_create_str(ctx, str, NULL);
}
void cubec_init_str_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_str_type_to_string,
      .get_length = &cubec_str_get_length,
      .get_index = &cubec_str_get_index,
      .to_string = &cubec_str_to_string,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_STR, sizeof(const char **),
                            sizeof(const char **), NULL, &opt, "str");
}
cubec_value_t cubec_create_str(cubec_context_t ctx, const char *data,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "str");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  const char *str = cubec_context_create_cstring(ctx, data);
  return cubec_context_create_value(ctx, type, false, &str, name);
}