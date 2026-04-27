#include "engine/void.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
void void_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .type_eq = type_default_eq,
  };
  type_t void_t =
      create_type(allocator, TYPE_KIND_VOID, 0, 0, "void", "void", &opt, NULL);
  context_store_type(ctx, void_t);
  create_type_value(ctx, void_t, false, "void");
}