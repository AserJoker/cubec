#include "engine/void.h"
#include "engine/context.h"
#include "engine/type.h"

void init_void_type(context_t ctx) {
  type_t type = create_type(ctx->allocator, TYPE_KIND_VOID, "void", "void", 0,
                            0, NULL, NULL);
  context_store_type(ctx, type);
}