#include "engine/wildcard_type.h"

type_t type_get_wildcard_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t wildcard_type = {
      .kind  = TYPE_KIND_WILDCARD,
      .name  = (char *)"?",
      .size  = 0,
      .align = 0,
      .mut   = false,
      .vtable = {0},
  };
  return &wildcard_type;
}
