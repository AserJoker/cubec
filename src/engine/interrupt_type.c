#include "engine/interrupt_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/type.h"
#include <string.h>

/* ---- Interrupt type vtable ---- */

static value_t _interrupt_clone(vm_t vm, value_t self) {
  struct interrupt_data_t *src = (struct interrupt_data_t *)value_get_data(self);
  return create_interrupt_value(vm, src->kind, src->value);
}

type_t type_get_interrupt_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_INTERRUPT,
      .name  = "interrupt",
      .size  = sizeof(struct interrupt_data_t),
      .align = _Alignof(struct interrupt_data_t),
      .mut   = false,
      .vtable = {
          .clone = _interrupt_clone,
          .equal = NULL,
          .extends = NULL,
          .type_equal = NULL,
          .type_extends = NULL,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

value_t create_interrupt_value(vm_t vm, interrupt_kind_t kind, value_t value) {
  allocator_t allocator = vm_get_allocator(vm);

  struct interrupt_data_t *data =
      (struct interrupt_data_t *)allocator_alloc(allocator,
                                                  sizeof(struct interrupt_data_t));
  data->kind  = kind;
  data->value = value; /* borrowed reference */

  type_t interrupt_type = (type_t)value_get_data(vm_get_interrupt_type(vm));
  value_t v = value_create(allocator, interrupt_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
  }
  return v;
}

interrupt_kind_t interrupt_get_kind(value_t self) {
  struct interrupt_data_t *data = (struct interrupt_data_t *)value_get_data(self);
  return data->kind;
}

value_t interrupt_get_value(value_t self) {
  struct interrupt_data_t *data = (struct interrupt_data_t *)value_get_data(self);
  return data->value;
}
