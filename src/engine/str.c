#include "engine/str.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
static char *cubec_str_type_to_string(cubec_type_t self,
                                      cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "str");
}
void cubec_init_str_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_str_type_to_string,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_STR, sizeof(const char **),
                            sizeof(const char **), NULL, &opt, "str");
}
cubec_value_t cubec_create_str(cubec_context_t ctx, const char *data,
                               bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "str");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  const char *str = cubec_context_create_cstring(ctx, data);
  return cubec_context_create_value(ctx, type, mutable, &str, name);
}