#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type_kind.h"
#include "engine/value.h"
static void cubec_union_type_dispose(cubec_union_type_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->items);
}
cubec_type_t cubec_create_union_type(cubec_allocator_t allocator,
                                     const char *name, cubec_array_t items) {
  cubec_union_type_t type =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_type_t),
                            (cubec_dispose_fn_t)cubec_union_type_dispose);
  type->super.kind = CUBEC_TYPE_KIND_UNION;
  type->super.name = name;
  return &type->super;
}
cubec_value_t cubec_create_union_value(cubec_allocator_t allocator,
                                       cubec_type_t type, cubec_value_t value) {
  cubec_union_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_union_value_t), NULL);
  self->super.type = type;
  self->value = value;
  return &self->super;
}
