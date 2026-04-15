#include "engine/void.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"

static char *void_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "void");
}
void init_void_type(context_t ctx) {
  struct _type_operator_t opt = {
      .type_to_string = &void_type_to_string,
  };
  context_create_type(ctx, CUBEC_VALUE_TYPE_VOID, 0, 0, NULL, &opt, "void");
}