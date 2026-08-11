#include "engine/void_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/type.h"

/* ---- Void type vtable ---- */

static value_t _void_clone(vm_t vm, value_t self) {
  (void)self;
  type_t t = (type_t)value_get_data(vm_get_void_type(vm));
  value_t v = value_create(vm_get_allocator(vm), t, NULL, false);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _void_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_VOID);
}

static value_t _void_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_VOID);
}

type_t type_get_void_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t void_type = {
      .kind  = TYPE_KIND_VOID,
      .name  = (char *)"void",
      .size  = 0,
      .align = 0,
      .mut   = false,
      .vtable = {
          .clone = _void_clone,
          .equal = NULL,
          .extends = NULL,
          .type_equal = _void_type_equal,
          .type_extends = _void_type_extends,
      },
  };
  return &void_type;
}

value_t create_void_value(vm_t vm) {
  allocator_t allocator = vm_get_allocator(vm);
  type_t void_type = (type_t)value_get_data(vm_get_void_type(vm));
  value_t v = value_create(allocator, void_type, NULL, false);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
  }
  return v;
}
