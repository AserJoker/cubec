#include "engine/bool_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/error_type.h"
#include "engine/type.h"
#include <stdbool.h>

/* ---- Bool type vtable ---- */

static value_t _bool_clone(allocator_t allocator, value_t self) {
  bool *src = (bool *)value_get_data(self);
  bool *copy = (bool *)allocator_alloc(allocator, sizeof(bool));
  *copy = *src;
  return value_create(allocator, value_get_type(self), copy, true);
}

static void _bool_dispose(allocator_t allocator, value_t self) {
  bool *d = (bool *)value_get_data(self);
  allocator_free(allocator, &d);
}

static value_t _bool_equal(vm_t vm, value_t a, value_t b) {
  type_t ta = value_get_type(a);
  type_t tb = value_get_type(b);
  if (ta->kind != tb->kind)
    return create_error_value(vm, "cannot compare values of different kinds");
  return create_bool_value(vm, *(bool *)value_get_data(a) == *(bool *)value_get_data(b));
}

static value_t _bool_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_BOOL);
}

static value_t _bool_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_BOOL);
}

type_t type_create_bool_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t bool_type = {
      .kind  = TYPE_KIND_BOOL,
      .name  = "bool",
      .size  = 1,
      .align = 1,
      .vtable = {
          .clone = _bool_clone,
          .dispose = _bool_dispose,
          .equal = _bool_equal,
          .extends = NULL,
          .type_equal = _bool_type_equal,
          .type_extends = _bool_type_extends,
      },
  };
  return &bool_type;
}

value_t create_bool_value(vm_t vm, bool val) {
  allocator_t allocator = vm_get_allocator(vm);
  bool *data = (bool *)allocator_alloc(allocator, sizeof(bool));
  *data = val;
  type_t bool_type = (type_t)value_get_data(vm_get_bool_type(vm));
  value_t v = value_create(allocator, bool_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
  }
  return v;
}
