#include "engine/builtin.h"
#include "core/allocator.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static char *cubec_built_type_to_string(cubec_type_t self,
                                        cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "builtin");
}
void cubec_init_builtin_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_built_type_to_string,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_BUILTIN,
                            sizeof(cubec_builtin_fn_t),
                            sizeof(cubec_builtin_fn_t), NULL, &opt, "builtin");
}
cubec_value_t cubec_create_builtin(cubec_context_t ctx, cubec_builtin_fn_t fn,
                                   bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "builtin");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, fn, name);
}