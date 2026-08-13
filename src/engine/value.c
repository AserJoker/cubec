#include "engine/value.h"
#include "engine/type.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/bool_type.h"
#include <string.h>

struct _value_t {
  type_t type;
  void  *data;
  bool   own;
  bool   initialized; /* false = TDZ (temporal dead zone), true = initialized */
};

typedef struct value_init_t {
  type_t type;
  void  *data;
  bool   own;
  bool   initialized;
} value_init_t;

static void _value_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  value_t v = (value_t)self;
  value_init_t *init = (value_init_t *)arg;
  v->type = init->type;
  v->data = init->data;
  v->own = init->own;
  v->initialized = init->initialized;
}

static void _value_dispose(void *self, allocator_t allocator) {
  value_t v = (value_t)self;
  if (v->own && v->data) {
    void *d = v->data;
    allocator_free(allocator, &d);
  }
  v->type = NULL;
  v->data = NULL;
  v->own = false;
  v->initialized = false;
}

static void _value_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  value_t dst = (value_t)self;
  value_t src = (value_t)another;
  dst->type = src->type;
  dst->data = src->data;
  dst->own = src->own;
  dst->initialized = true; /* move succeeds only on initialized values */
  src->data = NULL;
  src->own = false;
  src->initialized = false;
}

class_t g_value_class = {
    .size = sizeof(struct _value_t),
    .name = "cubec.engine.value",
    .init = (class_init_fn_t)_value_init,
    .dispose = (class_dispose_fn_t)_value_dispose,
    .clone = NULL,
    .move = (class_move_fn_t)_value_move,
};

value_t value_create(allocator_t allocator, type_t type, void *data,
                     bool own) {
  value_init_t init = {.type = type, .data = data, .own = own,
                        .initialized = (data != NULL)};
  return (value_t)allocator_create(allocator, &g_value_class, &init);
}

type_t  value_get_type(value_t self) { return self->type; }
void   *value_get_data(value_t self) { return self->data; }
bool    value_is_own(value_t self) { return self->own; }
bool    value_is_shadow(value_t self) { return self->data == NULL; }
bool    value_is_initialized(value_t self) { return self->initialized; }

void    value_set_initialized(value_t self, bool initialized) {
  self->initialized = initialized;
}

value_t value_clone(vm_t vm, value_t self) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.clone)
    return create_exception_value(vm, "type '%s' does not support clone",
                              type_get_name(value_get_type(self)));
  return vt.clone(vm, self);
}

type_t value_type_clone(vm_t vm, type_t self) {
  allocator_t alloc = vm_get_allocator(vm);
  type_t cloned = (type_t)alloc_clone(alloc, self);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->types, cloned);
  return cloned;
}

value_t value_deref_get(vm_t vm, value_t self) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.deref_get)
    return create_exception_value(vm, "type '%s' does not support dereference",
                              type_get_name(value_get_type(self)));
  return vt.deref_get(vm, self);
}

value_t value_deref_set(vm_t vm, value_t self, value_t val) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.deref_set)
    return create_exception_value(vm, "type '%s' does not support dereference assignment",
                              type_get_name(value_get_type(self)));
  return vt.deref_set(vm, self, val);
}

value_t value_equal(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.equal)
    return create_exception_value(vm, "type '%s' does not support operator ==",
                              type_get_name(value_get_type(a)));
  return vt.equal(vm, a, b);
}

value_t value_extends(vm_t vm, value_t sub, value_t super_val) {
  vtable_t vt = type_get_vtable(value_get_type(sub));
  if (!vt.extends)
    return create_exception_value(vm, "type '%s' does not support extends",
                              type_get_name(value_get_type(sub)));
  return vt.extends(vm, sub, super_val);
}

value_t value_band(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.band)
    return create_exception_value(vm, "type '%s' does not support operator &",
                              type_get_name(value_get_type(a)));
  return vt.band(vm, a, b);
}

value_t value_bor(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bor)
    return create_exception_value(vm, "type '%s' does not support operator |",
                              type_get_name(value_get_type(a)));
  return vt.bor(vm, a, b);
}

value_t value_bxor(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bxor)
    return create_exception_value(vm, "type '%s' does not support operator ^",
                              type_get_name(value_get_type(a)));
  return vt.bxor(vm, a, b);
}

value_t value_bnot(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.bnot)
    return create_exception_value(vm, "type '%s' does not support operator ~",
                              type_get_name(value_get_type(a)));
  return vt.bnot(vm, a);
}

value_t value_lnot(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.lnot)
    return create_exception_value(vm, "type '%s' does not support operator !",
                              type_get_name(value_get_type(a)));
  return vt.lnot(vm, a);
}

value_t value_pos(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.pos)
    return create_exception_value(vm, "type '%s' does not support operator +",
                              type_get_name(value_get_type(a)));
  return vt.pos(vm, a);
}

value_t value_neg(vm_t vm, value_t a) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.neg)
    return create_exception_value(vm, "type '%s' does not support operator -",
                              type_get_name(value_get_type(a)));
  return vt.neg(vm, a);
}

value_t value_safe_cast(vm_t vm, value_t val, type_t to) {
  vtable_t vt = type_get_vtable(value_get_type(val));
  if (!vt.safe_cast)
    return create_exception_value(vm, "type '%s' does not support safe_cast",
                              type_get_name(value_get_type(val)));
  return vt.safe_cast(vm, val, to);
}

value_t value_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  /* const check: initialized && !mut → cannot assign */
  if (value_is_initialized(lvalue) && !lt->mut)
    return create_exception_value(vm, "cannot assign to const '%s'", lt->name);
  vtable_t vt = type_get_vtable(lt);
  if (!vt.assignment)
    return create_exception_value(vm, "type '%s' does not support assignment",
                              type_get_name(lt));
  return vt.assignment(vm, lvalue, rvalue);
}

value_t value_add(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.add)
    return create_exception_value(vm, "type '%s' does not support operator +",
                              type_get_name(value_get_type(a)));
  return vt.add(vm, a, b);
}

value_t value_sub(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.sub)
    return create_exception_value(vm, "type '%s' does not support operator -",
                              type_get_name(value_get_type(a)));
  return vt.sub(vm, a, b);
}

value_t value_mul(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.mul)
    return create_exception_value(vm, "type '%s' does not support operator *",
                              type_get_name(value_get_type(a)));
  return vt.mul(vm, a, b);
}

value_t value_div(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.div)
    return create_exception_value(vm, "type '%s' does not support operator /",
                              type_get_name(value_get_type(a)));
  return vt.div(vm, a, b);
}

value_t value_mod(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.mod)
    return create_exception_value(vm, "type '%s' does not support operator %%",
                              type_get_name(value_get_type(a)));
  return vt.mod(vm, a, b);
}

value_t value_shl(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.shl)
    return create_exception_value(vm, "type '%s' does not support operator <<",
                              type_get_name(value_get_type(a)));
  return vt.shl(vm, a, b);
}

value_t value_shr(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.shr)
    return create_exception_value(vm, "type '%s' does not support operator >>",
                              type_get_name(value_get_type(a)));
  return vt.shr(vm, a, b);
}

value_t value_gt(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.gt)
    return create_exception_value(vm, "type '%s' does not support operator >",
                              type_get_name(value_get_type(a)));
  return vt.gt(vm, a, b);
}

value_t value_lt(vm_t vm, value_t a, value_t b) {
  vtable_t vt = type_get_vtable(value_get_type(a));
  if (!vt.lt)
    return create_exception_value(vm, "type '%s' does not support operator <",
                              type_get_name(value_get_type(a)));
  return vt.lt(vm, a, b);
}

/* != is derived from equal: negate the result */
value_t value_ne(vm_t vm, value_t a, value_t b) {
  value_t eq = value_equal(vm, a, b);
  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
    return eq;
  if (value_is_shadow(eq))
    return vm_create_value_shadow(vm, value_get_type(eq), NULL, true);
  bool result = !(*(bool *)value_get_data(eq));
  return create_bool_value(vm, result);
}

/* >= is derived from lt: negate the result */
value_t value_ge(vm_t vm, value_t a, value_t b) {
  value_t lt_result = value_lt(vm, a, b);
  if (type_get_kind(value_get_type(lt_result)) == TYPE_KIND_EXCEPTION)
    return lt_result;
  if (value_is_shadow(lt_result))
    return vm_create_value_shadow(vm, value_get_type(lt_result), NULL, true);
  bool result = !(*(bool *)value_get_data(lt_result));
  return create_bool_value(vm, result);
}

/* <= is derived from gt: negate the result */
value_t value_le(vm_t vm, value_t a, value_t b) {
  value_t gt_result = value_gt(vm, a, b);
  if (type_get_kind(value_get_type(gt_result)) == TYPE_KIND_EXCEPTION)
    return gt_result;
  if (value_is_shadow(gt_result))
    return vm_create_value_shadow(vm, value_get_type(gt_result), NULL, true);
  bool result = !(*(bool *)value_get_data(gt_result));
  return create_bool_value(vm, result);
}

value_t value_to_string(vm_t vm, value_t self) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.to_string)
    return create_exception_value(vm, "type '%s' does not support to_string",
                              type_get_name(value_get_type(self)));
  return vt.to_string(vm, self);
}

value_t value_get_field(vm_t vm, value_t self, const char *name) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.get_field)
    return create_exception_value(vm, "type '%s' does not support field access",
                              type_get_name(value_get_type(self)));
  return vt.get_field(vm, self, name);
}

value_t value_set_field(vm_t vm, value_t self, const char *name, value_t val) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.set_field)
    return create_exception_value(vm, "type '%s' does not support field assignment",
                              type_get_name(value_get_type(self)));
  return vt.set_field(vm, self, name, val);
}

value_t value_get_item(vm_t vm, value_t self, value_t index) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.get_item)
    return create_exception_value(vm, "type '%s' does not support subscript access",
                              type_get_name(value_get_type(self)));
  return vt.get_item(vm, self, index);
}

value_t value_set_item(vm_t vm, value_t self, value_t index, value_t val) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.set_item)
    return create_exception_value(vm, "type '%s' does not support subscript assignment",
                              type_get_name(value_get_type(self)));
  return vt.set_item(vm, self, index, val);
}

value_t value_slice(vm_t vm, value_t self, uint64_t start, uint64_t count) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.slice)
    return create_exception_value(vm, "type '%s' does not support slicing",
                              type_get_name(value_get_type(self)));
  return vt.slice(vm, self, start, count);
}

value_t value_call(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  vtable_t vt = type_get_vtable(value_get_type(fn));
  if (!vt.call)
    return create_exception_value(vm, "type '%s' is not callable",
                              type_get_name(value_get_type(fn)));
  return vt.call(vm, fn, argc, argv);
}

value_t value_member_addr(vm_t vm, value_t self, const char *name) {
  type_kind_t kind = type_get_kind(value_get_type(self));
  if (kind == TYPE_KIND_STRUCT) {
    extern value_t _struct_value_member_addr(vm_t vm, value_t self, const char *name);
    return _struct_value_member_addr(vm, self, name);
  }
  if (kind == TYPE_KIND_UNION) {
    extern value_t _union_value_member_addr(vm_t vm, value_t self, const char *name);
    return _union_value_member_addr(vm, self, name);
  }
  return create_exception_value(vm, "type '%s' does not support member address",
                            type_get_name(value_get_type(self)));
}

value_t value_member_call(vm_t vm, value_t self, const char *name,
                          size_t argc, value_t *argv) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.member_call)
    return create_exception_value(vm, "type '%s' does not support member call",
                              type_get_name(value_get_type(self)));
  return vt.member_call(vm, self, name, argc, argv);
}

value_t value_get_prop(vm_t vm, value_t self, const char *name) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.get_prop)
    return create_exception_value(vm, "type '%s' does not support static property access",
                              type_get_name(value_get_type(self)));
  return vt.get_prop(vm, self, name);
}

value_t value_set_prop(vm_t vm, value_t self, const char *name, value_t val) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.set_prop)
    return create_exception_value(vm, "type '%s' does not support static property assignment",
                              type_get_name(value_get_type(self)));
  return vt.set_prop(vm, self, name, val);
}

value_t value_is(vm_t vm, value_t self, type_t type) {
  vtable_t vt = type_get_vtable(value_get_type(self));
  if (!vt.is_instance)
    return create_exception_value(vm, "type '%s' does not support 'is' operator",
                              type_get_name(value_get_type(self)));
  return vt.is_instance(vm, self, type);
}
