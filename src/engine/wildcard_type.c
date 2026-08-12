#include "engine/wildcard_type.h"
#include "engine/type.h"

type_t type_get_wildcard_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_WILDCARD,
      .name  = "?",
      .size  = 0,
      .align = 0,
      .mut   = false,
      .vtable = {0},
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}
