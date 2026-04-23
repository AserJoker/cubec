#include "engine/void.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
static char *void_write_ast(value_t self, allocator_t allocator) {
  return create_cstring(allocator, "undefined");
}
void void_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .write_ast = void_write_ast,
  };
  type_t void_t =
      create_type(allocator, TYPE_KIND_VOID, 0, 0, "void", "void", &opt, NULL);
  context_store_type(ctx, void_t);
  create_type_value(ctx, void_t, false, true, "void");
}