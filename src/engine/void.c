#include "engine/void.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"

static char *cubec_void_type_to_string(cubec_type_t self,
                                       cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "void");
}
void cubec_init_void_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_void_type_to_string,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_VOID, 0, 0, NULL, &opt,
                            "void");
}