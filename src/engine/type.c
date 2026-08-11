#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/error_type.h"
#include "core/string.h"
#include <string.h>

type_kind_t type_get_kind(type_t self) { return self->kind; }
const char *type_get_name(type_t self) { return self->name; }
uint64_t    type_get_size(type_t self) { return self->size; }
uint64_t    type_get_align(type_t self) { return self->align; }
bool        type_is_mut(type_t self) { return self->mut; }
vtable_t    type_get_vtable(type_t self) { return self->vtable; }

/* ---- Bootstrap type "type" vtable ---- */

static value_t _type_clone(allocator_t allocator, value_t self) {
  type_t src = (type_t)value_get_data(self);
  type_t copy = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
  *copy = *src;
  copy->name = cstring_clone(allocator, src->name);
  return value_create(allocator, value_get_type(self), copy, true);
}

static void _type_dispose(allocator_t allocator, value_t self) {
  type_t t = (type_t)value_get_data(self);
  allocator_free(allocator, &t->name);
  allocator_free(allocator, &t);
}

static value_t _type_equal(vm_t vm, value_t a, value_t b) {
  type_t ta = (type_t)value_get_data(a);
  type_t tb = (type_t)value_get_data(b);
  if (ta->kind != tb->kind)
    return create_error_value(vm, "cannot compare types of different kinds: %s vs %s",
                              ta->name, tb->name);
  if (!ta->vtable.type_equal)
    return create_error_value(vm, "type '%s' does not support type_equal", ta->name);
  return ta->vtable.type_equal(vm, ta, tb);
}

static value_t _type_extends(vm_t vm, value_t sub, value_t super_val) {
  type_t t_sub = (type_t)value_get_data(sub);
  type_t t_super = (type_t)value_get_data(super_val);
  if (t_sub->kind != t_super->kind)
    return create_error_value(vm, "cannot check extends between types of different kinds: %s vs %s",
                              t_sub->name, t_super->name);
  if (!t_sub->vtable.type_extends)
    return create_error_value(vm, "type '%s' does not support type_extends", t_sub->name);
  return t_sub->vtable.type_extends(vm, t_sub, t_super);
}

type_t type_get_type_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t type_type = {
      .kind  = TYPE_KIND_TYPE,
      .name  = (char *)"type",
      .size  = sizeof(struct _type_t),
      .align = _Alignof(struct _type_t),
      .mut   = false,
      .vtable = {
          .clone = _type_clone,
          .dispose = _type_dispose,
          .equal = _type_equal,
          .extends = _type_extends,
          .type_equal = NULL,
          .type_extends = NULL,
      },
  };
  return &type_type;
}

value_t create_type_value(vm_t vm, type_t type, const char *name, bool own) {
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  allocator_t allocator = vm_get_allocator(vm);
  value_t v = value_create(allocator, type_type, type, own);
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

