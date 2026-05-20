#include "engine/void.h"
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <string.h>

void init_void_type(context_t ctx) {
  type_t type = create_type(ctx->allocator, TYPE_KIND_VOID, "void", "void", 0,
                            0, NULL, NULL);
  context_store_type(ctx, type);
}
value_t create_comptime_void(context_t ctx) {
  type_t type = context_load_type(ctx, "void");
  return context_create_comptime_value(ctx, type, NULL, false, NULL);
}