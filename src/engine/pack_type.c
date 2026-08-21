#include "engine/pack_type.h"
#include "engine/type.h"
#include "engine/bool_type.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "core/string.h"
#include "core/vec.h"

/* ---- Pack type vtable ---- */

static value_t _pack_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  return create_bool_value(vm, b->kind == TYPE_KIND_PACK);
}

static value_t _pack_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_PACK);
}

static type_t _pack_type_type_clone(vm_t vm, type_t self) {
  allocator_t allocator = vm_get_allocator(vm);
  type_t dst = (type_t)allocator_create(allocator, &g_type_class, NULL);

  dst->kind   = self->kind;
  dst->name   = self->name ? cstring_clone(allocator, self->name) : NULL;
  dst->size   = self->size;
  dst->align  = self->align;
  dst->mut    = self->mut;
  dst->vtable = self->vtable;

  vec_push(vm_get_types(vm), dst);
  return dst;
}

type_t type_get_pack_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_PACK,
      .name  = "...type",
      .size  = 0,
      .align = 0,
      .mut   = false,
      .vtable = {
          .type_equal   = _pack_type_equal,
          .type_extends = _pack_type_extends,
          .type_clone   = _pack_type_type_clone,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}
