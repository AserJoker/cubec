#include "engine/void.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
void void_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_t void_t =
      create_type(allocator, TYPE_KIND_VOID, 0, 0, "void", "void", NULL, NULL);
  context_store_type(ctx, void_t);
  create_type_value(ctx, void_t, false, true, "void");
  context_create_value(ctx, void_t, NULL, false, true, "undefined");
}