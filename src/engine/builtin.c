#include "engine/builtin.h"
#include "core/allocator.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static char *builtin_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "builtin");
}
static value_t builtin_call(value_t self, context_t ctx, size_t argc,
                            value_t argv[]) {
  builtin_fn_t fn = *(builtin_fn_t *)value_get_data(self);
  return fn(ctx, argc, argv);
}
void init_builtin_type(context_t ctx) {
  struct _type_operator_t opt = {
      .type_to_string = &builtin_type_to_string,
      .call = &builtin_call,
  };
  context_create_type(ctx, VALUE_TYPE_BUILTIN, sizeof(builtin_fn_t),
                      sizeof(builtin_fn_t), NULL, &opt, "builtin");
}
value_t create_builtin(context_t ctx, builtin_fn_t fn, const char *name) {
  value_t vtype = context_load(ctx, "builtin");
  type_t type = *(type_t *)value_get_data(vtype);
  value_t val = context_create_value(ctx, type, false, &fn, name);
  value_set_comptime(val, true);
  return val;
}