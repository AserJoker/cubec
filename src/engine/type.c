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

/* ---- g_type_class: lifecycle for dynamically-created type_t ---- */

static void _gtype_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  type_t t = (type_t)self;
  type_init_t *init = (type_init_t *)arg;
  t->kind  = init->kind;
  t->name  = cstring_clone(allocator, init->name);
  t->size  = init->size;
  t->align = init->align;
  t->mut   = init->mut;
  t->vtable = init->vtable;
}

static void _gtype_dispose(void *self, allocator_t allocator) {
  type_t t = (type_t)self;
  allocator_free(allocator, &t->name);
  t->name = NULL;
}

static void _gtype_clone(void *self, allocator_t allocator, void *another) {
  type_t dst = (type_t)self;
  type_t src = (type_t)another;
  dst->kind   = src->kind;
  dst->name   = cstring_clone(allocator, src->name);
  dst->size   = src->size;
  dst->align  = src->align;
  dst->mut    = src->mut;
  dst->vtable = src->vtable;
}

static void _gtype_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  type_t dst = (type_t)self;
  type_t src = (type_t)another;
  *dst = *src;
  src->name = NULL;
}

class_t g_type_class = {
    .size    = sizeof(struct _type_t),
    .name    = "cubec.engine.type",
    .init    = (class_init_fn_t)_gtype_init,
    .dispose = (class_dispose_fn_t)_gtype_dispose,
    .clone   = (class_clone_fn_t)_gtype_clone,
    .move    = (class_move_fn_t)_gtype_move,
};

type_t type_create(allocator_t allocator, type_kind_t kind, const char *name,
                   uint64_t size, uint64_t align, bool mut, vtable_t vtable) {
  type_init_t init = {
      .kind = kind, .name = name, .size = size,
      .align = align, .mut = mut, .vtable = vtable,
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

/* ---- Bootstrap type "type" vtable ---- */

static value_t _type_clone(vm_t vm, value_t self) {
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  type_t inner = (type_t)value_get_data(self);
  allocator_t allocator = vm_get_allocator(vm);
  /* Clone the inner type_t via g_type_class */
  type_t cloned_inner = (type_t)alloc_clone(allocator, inner);
  value_t v = value_create(allocator, type_type, cloned_inner, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _type_equal(vm_t vm, value_t a, value_t b) {
  type_t ta = (type_t)value_get_data(a);
  type_t tb = (type_t)value_get_data(b);
  /* wildcard short-circuit: any type equal to wildcard */
  if (tb->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
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
  /* wildcard short-circuit: any type extends wildcard */
  if (t_super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (t_sub->kind != t_super->kind)
    return create_error_value(vm, "cannot check extends between types of different kinds: %s vs %s",
                              t_sub->name, t_super->name);
  if (!t_sub->vtable.type_extends)
    return create_error_value(vm, "type '%s' does not support type_extends", t_sub->name);
  return t_sub->vtable.type_extends(vm, t_sub, t_super);
}

static value_t _type_get_prop(vm_t vm, value_t self, const char *name) {
  type_t inner = (type_t)value_get_data(self);
  if (!inner->vtable.type_get_prop)
    return create_error_value(vm, "type '%s' does not support static property access",
                              inner->name);
  return inner->vtable.type_get_prop(vm, inner, name);
}

static value_t _type_set_prop(vm_t vm, value_t self, const char *name, value_t val) {
  type_t inner = (type_t)value_get_data(self);
  if (!inner->vtable.type_set_prop)
    return create_error_value(vm, "type '%s' does not support static property assignment",
                              inner->name);
  return inner->vtable.type_set_prop(vm, inner, name, val);
}

type_t type_get_type_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_TYPE,
      .name  = "type",
      .size  = sizeof(struct _type_t),
      .align = _Alignof(struct _type_t),
      .mut   = false,
      .vtable = {
          .clone = _type_clone,
          .equal = _type_equal,
          .extends = _type_extends,
          .type_equal = NULL,
          .type_extends = NULL,
          .get_prop = _type_get_prop,
          .set_prop = _type_set_prop,
          .type_get_prop = NULL,
          .type_set_prop = NULL,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
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

