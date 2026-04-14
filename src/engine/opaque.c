#include "engine/opaque.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static char *cubec_opaque_type_to_string(cubec_type_t self,
                                         cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "opaque");
}
static cubec_value_t cubec_opaque_convert(cubec_value_t self,
                                          cubec_context_t ctx,
                                          cubec_type_t type) {
  if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_PTR ||
      cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_PARRAY) {
    void *data = *(void **)cubec_value_get_data(self);
    return cubec_context_create_value(ctx, type, false, &data, NULL);
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *type_name = cubec_type_to_string(type, allocator);
  cubec_value_t err =
      cubec_create_error(ctx, "cannot convert opaque to %s", type_name);
  cubec_allocator_free(allocator, type_name);
  return err;
}
void cubec_init_opaque_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_opaque_type_to_string,
      .convert = cubec_opaque_convert,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_OPAQUE, sizeof(void *),
                            sizeof(void *), NULL, &opt, "opaque");
}
cubec_value_t cubec_create_opaque(cubec_context_t ctx, const void *data,
                                  bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "opaque");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &data, name);
}