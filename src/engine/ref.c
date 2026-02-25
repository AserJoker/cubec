#include "engine/ref.h"
#include "core/allocator.h"
#include "engine/type_kind.h"
#include "engine/value.h"
cubec_type_t cubec_create_ref_type(cubec_allocator_t allocator,
                                   const char *name, cubec_type_t base_type) {
  cubec_ref_type_t type =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_type_t), NULL);
  type->base_type = base_type;
  type->super.kind = CUBEC_TYPE_KIND_REF;
  type->super.name = name;
  return &type->super;
}
cubec_value_t cubec_create_ref_value(cubec_allocator_t allocator,
                                     cubec_type_t type,
                                     cubec_value_t base_value) {
  if (type->kind != CUBEC_TYPE_KIND_REF) {
    return NULL;
  }
  if (base_value->type->kind == CUBEC_TYPE_KIND_REF) {
    return cubec_create_ref_value(allocator, type,
                                  cubec_ref_value_get(base_value));
  }
  cubec_ref_value_t value =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_value_t), NULL);
  value->super.type = type;
  value->value = base_value;
  return &value->super;
}
cubec_value_t cubec_ref_value_get(cubec_value_t value) {
  if (value->type->kind != CUBEC_TYPE_KIND_REF) {
    return NULL;
  }
  cubec_ref_value_t ref = (cubec_ref_value_t)value;
  return ref->value;
}