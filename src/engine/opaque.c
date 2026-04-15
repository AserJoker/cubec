#include "engine/opaque.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static char *opaque_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "opaque");
}
static value_t opaque_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == VALUE_TYPE_PTR ||
      type_get_kind(type) == VALUE_TYPE_PARRAY) {
    void *data = *(void **)value_get_data(self);
    return context_create_value(ctx, type, false, &data, NULL);
  }
  allocator_t allocator = context_get_allocator(ctx);
  char *type_name = type_to_string(type, allocator);
  value_t err = create_error(ctx, "cannot convert opaque to %s", type_name);
  allocator_free(allocator, type_name);
  return err;
}
void init_opaque_type(context_t ctx) {
  struct _type_operator_t opt = {
      .type_to_string = &opaque_type_to_string,
      .convert = opaque_convert,
  };
  context_create_type(ctx, VALUE_TYPE_OPAQUE, sizeof(void *), sizeof(void *),
                      NULL, &opt, "opaque");
}
value_t create_opaque(context_t ctx, const void *data, bool mutable,
                      const char *name) {
  value_t vtype = context_load(ctx, "opaque");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &data, name);
}