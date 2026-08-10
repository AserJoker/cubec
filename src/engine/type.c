#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/string.h"
#include <string.h>

type_kind_t type_get_kind(type_t self) { return self->kind; }
const char *type_get_name(type_t self) { return self->name; }
uint64_t    type_get_size(type_t self) { return self->size; }
uint64_t    type_get_align(type_t self) { return self->align; }
vtable_t    type_get_vtable(type_t self) { return self->vtable; }

/* ---- Bootstrap type "type" vtable ---- */

static value_t _type_clone(allocator_t allocator, value_t self) {
  type_t src = (type_t)value_get_data(self);
  type_t copy = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
  *copy = *src;
  return value_create(allocator, value_get_type(self), copy, true);
}

static void _type_dispose(allocator_t allocator, value_t self) {
  type_t t = (type_t)value_get_data(self);
  allocator_free(allocator, &t);
}

static bool _type_equal(value_t a, value_t b) {
  type_t ta = (type_t)value_get_data(a);
  type_t tb = (type_t)value_get_data(b);
  if (!ta->vtable.type_equal) return ta == tb;
  return ta->vtable.type_equal(ta, tb);
}

static bool _type_extends(value_t sub, value_t super_val) {
  type_t t_sub = (type_t)value_get_data(sub);
  type_t t_super = (type_t)value_get_data(super_val);
  if (!t_sub->vtable.type_extends) return t_sub == t_super;
  return t_sub->vtable.type_extends(t_sub, t_super);
}

type_t type_create_type_type(allocator_t allocator) {
  type_t t = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
  t->kind  = TYPE_KIND_TYPE;
  t->name  = "type";
  t->size  = sizeof(struct _type_t);
  t->align = _Alignof(struct _type_t);
  t->vtable = (vtable_t){
      .clone = _type_clone,
      .dispose = _type_dispose,
      .equal = _type_equal,
      .extends = _type_extends,
      .type_equal = NULL,
      .type_extends = NULL,
  };
  return t;
}

value_t create_type_value(vm_t vm, type_t type, const char *name) {
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  allocator_t allocator = vm_get_allocator(vm);
  value_t v = value_create(allocator, type_type, type, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
    if (name) {
      name_t n = name_create(scope->allocator, v);
      char *owned_name = cstring_clone(scope->allocator, name);
      strmap_insert(scope->names, owned_name, n);
      allocator_free(scope->allocator, &owned_name);
    }
  }
  return v;
}
