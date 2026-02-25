#include "engine/enum.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/map.h"
#include "engine/type_kind.h"
#include <string.h>

static void cubec_enum_type_dispose(cubec_enum_type_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->options);
}

cubec_type_t cubec_create_enum_type(cubec_allocator_t allocator,
                                    const char *name, cubec_type_t base_type) {
  cubec_enum_type_t enu =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_type_t),
                            (cubec_dispose_fn_t)cubec_enum_type_dispose);
  enu->super.name = name;
  enu->super.kind = CUBEC_TYPE_KIND_ENUM;
  enu->base_type = base_type;
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  enu->options = cubec_create_map(allocator, &initialize);
  return &enu->super;
}
cubec_value_t cubec_create_enum_value(cubec_allocator_t allocator,
                                      cubec_type_t type, const char *option) {
  cubec_enum_value_t value = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_enum_value_t), NULL);
  value->super.type = type;
  value->option = option;
  return &value->super;
}
cubec_value_t cubec_enum_value_get(cubec_value_t value) {
  if (value->type->kind != CUBEC_TYPE_KIND_ENUM) {
    return NULL;
  }
  cubec_enum_value_t evalue = (cubec_enum_value_t)value;
  const char *option = evalue->option;
  cubec_enum_type_t etype = (cubec_enum_type_t)value->type;
  return cubec_map_get(etype->options, option, NULL);
}