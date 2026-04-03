#include "engine/opaque.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
static char *cubec_opaque_type_to_string(cubec_type_t self,
                                         cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "opaque");
}
void cubec_init_opaque_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_opaque_type_to_string,
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