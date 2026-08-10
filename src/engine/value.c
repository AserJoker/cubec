#include "engine/value.h"
#include "engine/type.h"
#include "engine/vm.h"
#include "engine/error_type.h"
#include <string.h>

struct _value_t {
  type_t type;
  void  *data;
  bool   own;
};

typedef struct value_init_t {
  type_t type;
  void  *data;
  bool   own;
} value_init_t;

static void _value_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  value_t v = (value_t)self;
  value_init_t *init = (value_init_t *)arg;
  v->type = init->type;
  v->data = init->data;
  v->own = init->own;
}

static void _value_dispose(void *self, allocator_t allocator) {
  value_t v = (value_t)self;
  if (v->own && v->data && type_get_vtable(v->type).dispose) {
    type_get_vtable(v->type).dispose(allocator, v);
  }
  v->type = NULL;
  v->data = NULL;
  v->own = false;
}

static void _value_clone(void *self, allocator_t allocator, void *another) {
  value_t dst = (value_t)self;
  value_t src = (value_t)another;
  dst->type = src->type;
  dst->own = false;
  dst->data = NULL;
  vtable_t vt = type_get_vtable(src->type);
  if (vt.clone) {
    value_t cloned = vt.clone(allocator, src);
    dst->data = value_get_data(cloned);
    dst->own = value_is_own(cloned);
    cloned->own = false;
    cloned->data = NULL;
    allocator_free(allocator, &cloned);
  }
}

static void _value_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  value_t dst = (value_t)self;
  value_t src = (value_t)another;
  dst->type = src->type;
  dst->data = src->data;
  dst->own = src->own;
  src->data = NULL;
  src->own = false;
}

class_t g_value_class = {
    .size = sizeof(struct _value_t),
    .name = "cubec.engine.value",
    .init = (class_init_fn_t)_value_init,
    .dispose = (class_dispose_fn_t)_value_dispose,
    .clone = (class_clone_fn_t)_value_clone,
    .move = (class_move_fn_t)_value_move,
};

value_t value_create(allocator_t allocator, type_t type, void *data,
                     bool own) {
  value_init_t init = {.type = type, .data = data, .own = own};
  return (value_t)allocator_create(allocator, &g_value_class, &init);
}

void value_dispose(value_t self, allocator_t allocator) {
  if (!self) return;
  allocator_free(allocator, &self);
}

type_t  value_get_type(value_t self) { return self->type; }
void   *value_get_data(value_t self) { return self->data; }
bool    value_is_own(value_t self) { return self->own; }
bool    value_is_shadow(value_t self) { return self->data == NULL; }

value_t value_equal(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.equal)
    return create_error_value(vm, "type '%s' does not support operator ==",
                              type_get_name(value_get_type(a)));
  return vt.equal(vm, a, b);
}

value_t value_extends(vm_t vm, value_t sub, value_t super_val) {
  vtable_t vt = type_get_vtable(value_get_type(sub));
  if (!vt.extends)
    return create_error_value(vm, "type '%s' does not support extends",
                              type_get_name(value_get_type(sub)));
  return vt.extends(vm, sub, super_val);
}

value_t value_band(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.band)
    return create_error_value(vm, "type '%s' does not support operator &",
                              type_get_name(value_get_type(a)));
  return vt.band(vm, a, b);
}

value_t value_bor(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bor)
    return create_error_value(vm, "type '%s' does not support operator |",
                              type_get_name(value_get_type(a)));
  return vt.bor(vm, a, b);
}

value_t value_bxor(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bxor)
    return create_error_value(vm, "type '%s' does not support operator ^",
                              type_get_name(value_get_type(a)));
  return vt.bxor(vm, a, b);
}

value_t value_bnot(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bnot)
    return create_error_value(vm, "type '%s' does not support operator ~",
                              type_get_name(value_get_type(a)));
  return vt.bnot(vm, a);
}

value_t value_lnot(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.lnot)
    return create_error_value(vm, "type '%s' does not support operator !",
                              type_get_name(value_get_type(a)));
  return vt.lnot(vm, a);
}
