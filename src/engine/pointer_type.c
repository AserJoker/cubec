#include "engine/pointer_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for vtable functions ---- */

static value_t _pointer_clone(vm_t vm, value_t self);
static value_t _pointer_equal(vm_t vm, value_t a, value_t b);
static value_t _pointer_type_equal(vm_t vm, type_t a, type_t b);
static value_t _pointer_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _pointer_deref_get(vm_t vm, value_t self);
static value_t _pointer_deref_set(vm_t vm, value_t self, value_t val);
static value_t _pointer_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _pointer_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _pointer_to_string(vm_t vm, value_t self);
static value_t _pointer_get_field(vm_t vm, value_t self, const char *name);
static value_t _pointer_set_field(vm_t vm, value_t self, const char *name, value_t val);
static value_t _pointer_get_field_raw(vm_t vm, value_t self, const char *name);
static value_t _pointer_member_call(vm_t vm, value_t self, const char *name,
                                     size_t argc, value_t *argv);
static value_t _pointer_is_instance(vm_t vm, value_t self, type_t type);

static vtable_t _make_pointer_vtable(void) {
  return (vtable_t){
      .clone        = _pointer_clone,
      .equal        = _pointer_equal,
      .extends      = NULL,
      .type_equal   = _pointer_type_equal,
      .type_extends = _pointer_type_extends,
      .band         = NULL,
      .bor          = NULL,
      .bxor         = NULL,
      .add          = NULL,
      .sub          = NULL,
      .mul          = NULL,
      .div          = NULL,
      .mod          = NULL,
      .shl          = NULL,
      .shr          = NULL,
      .gt           = NULL,
      .lt           = NULL,
      .bnot         = NULL,
      .lnot         = NULL,
      .pos          = NULL,
      .neg          = NULL,
      .safe_cast    = _pointer_safe_cast,
      .assignment   = _pointer_assignment,
      .to_string    = _pointer_to_string,
      .get_field    = _pointer_get_field,
      .set_field    = _pointer_set_field,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = _pointer_deref_get,
      .deref_set    = _pointer_deref_set,
      .slice        = NULL,
      .call         = NULL,
      .member_call  = _pointer_member_call,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .is_instance  = _pointer_is_instance,
      .get_field_raw= _pointer_get_field_raw,
  };
}

/* ---- Pointer type class ---- */

static void _pointer_type_init(void *self, allocator_t allocator, void *arg) {
  pointer_type_t pt = (pointer_type_t)self;
  pointer_type_init_t *init = (pointer_type_init_t *)arg;

  pt->base.kind    = init->kind;
  pt->base.name    = init->name ? cstring_clone(allocator, init->name) : NULL;
  pt->base.size    = init->size;
  pt->base.align   = init->align;
  pt->base.mut     = init->mut;
  pt->base.vtable  = init->vtable;

  pt->pointee_type = (type_t)alloc_clone(allocator, init->pointee_type);
  pt->is_volatile  = init->is_volatile;
}

static void _pointer_type_dispose(void *self, allocator_t allocator) {
  pointer_type_t pt = (pointer_type_t)self;
  allocator_free(allocator, &pt->pointee_type);
  if (pt->base.name) {
    void *p = pt->base.name;
    allocator_free(allocator, &p);
    pt->base.name = NULL;
  }
}

static void _pointer_type_clone(void *self, allocator_t allocator, void *another) {
  pointer_type_t dst = (pointer_type_t)self;
  pointer_type_t src = (pointer_type_t)another;

  dst->base.kind    = src->base.kind;
  dst->base.name    = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;

  dst->pointee_type = (type_t)alloc_clone(allocator, src->pointee_type);
  dst->is_volatile  = src->is_volatile;
}

class_t g_pointer_type_class = {
    .size    = sizeof(struct _pointer_type_t),
    .name    = "cubec.engine.pointer_type",
    .init    = (class_init_fn_t)_pointer_type_init,
    .dispose = (class_dispose_fn_t)_pointer_type_dispose,
    .clone   = (class_clone_fn_t)_pointer_type_clone,
    .move    = NULL,
};

/* ---- Type creation ---- */

pointer_type_t pointer_type_create(allocator_t allocator, type_t pointee_type,
                                    bool mut, bool is_volatile) {
  /* generate name: const? * volatile? T */
  const char *pn = type_get_name(pointee_type);
  size_t pn_len = strlen(pn);
  size_t name_cap = pn_len + 24;
  char *name = (char *)allocator_alloc(allocator, name_cap);
  size_t pos = 0;

  if (!mut) {
    name[pos++] = 'c';
    name[pos++] = 'o';
    name[pos++] = 'n';
    name[pos++] = 's';
    name[pos++] = 't';
    name[pos++] = ' ';
  }
  name[pos++] = '*';
  if (is_volatile) {
    name[pos++] = ' ';
    name[pos++] = 'v';
    name[pos++] = 'o';
    name[pos++] = 'l';
    name[pos++] = 'a';
    name[pos++] = 't';
    name[pos++] = 'i';
    name[pos++] = 'l';
    name[pos++] = 'e';
  }
  name[pos++] = ' ';
  memcpy(name + pos, pn, pn_len);
  pos += pn_len;
  name[pos] = '\0';

  pointer_type_init_t init = {
      .kind         = TYPE_KIND_POINTER,
      .name         = name,
      .size         = sizeof(void *),
      .align        = _Alignof(void *),
      .mut          = mut,
      .vtable       = _make_pointer_vtable(),
      .pointee_type = pointee_type,
      .is_volatile  = is_volatile,
  };

  pointer_type_t pt = (pointer_type_t)allocator_create(
      allocator, &g_pointer_type_class, &init);

  allocator_free(allocator, &name);
  return pt;
}

/* ---- Accessors ---- */

type_t pointer_type_get_pointee_type(pointer_type_t self) {
  return self->pointee_type;
}
bool pointer_type_is_volatile(pointer_type_t self) {
  return self->is_volatile;
}

/* ---- Value constructors ---- */

value_t create_pointer_value(vm_t vm, pointer_type_t pt, value_t pointee) {
  allocator_t alloc = vm_get_allocator(vm);
  void **data = (void **)allocator_alloc(alloc, sizeof(void *));
  *data = value_get_data(pointee);
  value_t v = value_create(alloc, (type_t)pt, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_pointer_value_from_addr(vm_t vm, pointer_type_t pt, void *addr) {
  allocator_t alloc = vm_get_allocator(vm);
  void **data = (void **)allocator_alloc(alloc, sizeof(void *));
  *data = addr;
  value_t v = value_create(alloc, (type_t)pt, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_pointer_shadow(vm_t vm, pointer_type_t pt, bool initialized) {
  return vm_create_value_shadow(vm, (type_t)pt, NULL, initialized);
}

/* ---- Address-of utility ---- */

value_t value_addrof(vm_t vm, value_t target) {
  type_kind_t kind = type_get_kind(value_get_type(target));
  /* void/type/error have no addressable data */
  if (kind == TYPE_KIND_VOID || kind == TYPE_KIND_TYPE || kind == TYPE_KIND_EXCEPTION || kind == TYPE_KIND_INTERRUPT)
    return create_exception_value(vm, "cannot take address of type '%s'",
                              type_get_name(value_get_type(target)));

  allocator_t alloc = vm_get_allocator(vm);
  /* pointer is always mutable by default — const pointer requires explicit type annotation */
  pointer_type_t pt = pointer_type_create(alloc, value_get_type(target),
                                           true, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->types, pt);

  /* shadow target → shadow pointer (compile-time type-only) */
  if (value_is_shadow(target))
    return create_pointer_shadow(vm, pt, value_is_initialized(target));

  return create_pointer_value(vm, pt, target);
}

/* ---- VTable: clone ---- */

static value_t _pointer_clone(vm_t vm, value_t self) {
  pointer_type_t pt = (pointer_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return create_pointer_shadow(vm, pt, value_is_initialized(self));

  type_t cloned_type = value_type_clone(vm, (type_t)pt);
  pointer_type_t dst_pt = (pointer_type_t)cloned_type;

  void **src_data = (void **)value_get_data(self);
  return create_pointer_value_from_addr(vm, dst_pt, *src_data);
}

/* ---- VTable: equal ---- */

static value_t _pointer_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_POINTER)
    return create_bool_value(vm, false);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  /* compare addresses */
  void **pa = (void **)value_get_data(a);
  void **pb = (void **)value_get_data(b);
  return create_bool_value(vm, *pa == *pb);
}

/* ---- VTable: type_equal ---- */

static value_t _pointer_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_POINTER)
    return create_bool_value(vm, false);

  pointer_type_t pa = (pointer_type_t)a;
  pointer_type_t pb = (pointer_type_t)b;

  /* volatile is silently ignored in equals */
  /* mut must match: const* != * */
  if (a->mut != b->mut)
    return create_bool_value(vm, false);

  /* delegate to pointee type's type_equal */
  type_t ea = pa->pointee_type;
  type_t eb = pb->pointee_type;
  if (type_get_kind(ea) == TYPE_KIND_WILDCARD || type_get_kind(eb) == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  vtable_t evt = type_get_vtable(ea);
  if (evt.type_equal)
    return evt.type_equal(vm, ea, eb);
  return create_bool_value(vm, type_get_kind(ea) == type_get_kind(eb));
}

/* ---- VTable: type_extends ---- */

static value_t _pointer_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_POINTER)
    return create_bool_value(vm, false);

  pointer_type_t sub_pt = (pointer_type_t)sub;
  pointer_type_t super_pt = (pointer_type_t)super;

  /* volatile is silently ignored in extends */
  if (sub->mut != super->mut)
    return create_bool_value(vm, false);

  /* delegate to pointee type's type_extends */
  type_t ea = sub_pt->pointee_type;
  type_t eb = super_pt->pointee_type;
  if (type_get_kind(eb) == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  vtable_t evt = type_get_vtable(ea);
  if (evt.type_extends)
    return evt.type_extends(vm, ea, eb);
  return create_bool_value(vm, type_get_kind(ea) == type_get_kind(eb));
}

/* ---- VTable: deref_get ---- */

static value_t _pointer_deref_get(vm_t vm, value_t self) {
  pointer_type_t pt = (pointer_type_t)value_get_type(self);

  /* shadow pointer → shadow of pointee type (compile-time type-only) */
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, pt->pointee_type, NULL, value_is_initialized(self));

  if (!value_get_data(self))
    return create_exception_value(vm, "cannot dereference null pointer");

  void **ptr = (void **)value_get_data(self);
  if (!*ptr)
    return create_exception_value(vm, "null pointer dereference");

  type_t pointee = pt->pointee_type;
  /* create a value referencing the pointee's data (borrowed, not owned) */
  allocator_t alloc = vm_get_allocator(vm);
  value_t result = value_create(alloc, pointee, *ptr, false);
  value_set_initialized(result, true);
  /* register in scope so it gets disposed (data is borrowed, own=false) */
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, result);
  return result;
}

/* ---- VTable: deref_set ---- */

static value_t _pointer_deref_set(vm_t vm, value_t self, value_t val) {
  pointer_type_t pt = (pointer_type_t)value_get_type(self);

  if (value_is_shadow(self)) {
    value_set_initialized(self, true);
    return create_void_value(vm);
  }
  if (!value_get_data(self))
    return create_exception_value(vm, "cannot dereference null/uninitialized pointer");

  /* check const pointer */
  if (!type_is_mut((type_t)pt))
    return create_exception_value(vm, "cannot write through const pointer");

  void **ptr = (void **)value_get_data(self);
  if (!*ptr)
    return create_exception_value(vm, "null pointer dereference");

  /* safe_cast val to pointee type, then memcpy */
  type_t pointee = pt->pointee_type;
  value_t casted = value_safe_cast(vm, val, pointee);
  if (value_is_error(casted))
    return casted;

  uint64_t size = type_get_size(pointee);
  if (size > 0 && value_get_data(casted))
    memcpy(*ptr, value_get_data(casted), (size_t)size);

  return create_void_value(vm);
}

/* ---- VTable: safe_cast ---- */

static value_t _pointer_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);

  /* wildcard accepts anything */
  if (to->kind == TYPE_KIND_WILDCARD)
    return self;

  /* must cast to pointer type */
  if (to->kind != TYPE_KIND_POINTER)
    return create_exception_value(vm, "cannot safe_cast pointer to '%s'", to->name);

  /* same type → identity */
  if (from == to)
    return self;

  pointer_type_t from_pt = (pointer_type_t)from;
  pointer_type_t to_pt   = (pointer_type_t)to;

  /* pointer mut is not checked here: const on the pointer variable
   * doesn't affect the pointee. Copying address from const *T to *T is safe.
   * Pointee const is handled by type_extends below (*const T ↛ *T). */

  /* pointee covariance: from.pointee must extend to.pointee */
  type_t from_elem = from_pt->pointee_type;
  type_t to_elem   = to_pt->pointee_type;
  vtable_t evt = type_get_vtable(from_elem);
  value_t ext;
  if (evt.type_extends)
    ext = evt.type_extends(vm, from_elem, to_elem);
  else if (to_elem->kind == TYPE_KIND_WILDCARD)
    ext = create_bool_value(vm, true);
  else
    ext = create_bool_value(vm, type_get_kind(from_elem) == type_get_kind(to_elem));

  if (value_is_error(ext))
    return ext;
  if (value_is_shadow(ext) || !(*(bool *)value_get_data(ext)))
    return create_exception_value(vm, "cannot safe_cast '%s' to '%s'",
                              type_get_name(from), type_get_name(to));

  /* cast accepted: create new pointer value with target type */
  if (value_is_shadow(self))
    return create_pointer_shadow(vm, to_pt, value_is_initialized(self));

  void **src_data = (void **)value_get_data(self);
  /* clone pointer type into current scope */
  type_t cloned_type = value_type_clone(vm, to);
  pointer_type_t dst_pt = (pointer_type_t)cloned_type;
  return create_pointer_value_from_addr(vm, dst_pt, *src_data);
}

/* ---- VTable: assignment ---- */

static value_t _pointer_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  type_t rt = value_get_type(rvalue);
  if (type_get_kind(rt) != TYPE_KIND_POINTER)
    return create_exception_value(vm, "cannot assign non-pointer to pointer");

  /* check const pointer */
  if (value_is_initialized(lvalue) && !type_is_mut(lt))
    return create_exception_value(vm, "cannot assign to const pointer");

  /* check rvalue safe_cast to lvalue type (handles pointee extends) */
  value_t casted = _pointer_safe_cast(vm, rvalue, lt);
  if (value_is_error(casted))
    return casted;

  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }

  void **src = (void **)value_get_data(rvalue);
  void **dst = (void **)value_get_data(lvalue);
  *dst = *src;
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: to_string ---- */

static value_t _pointer_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_str_type(vm)), NULL, true);
  type_t t = value_get_type(self);
  const char *type_name = type_get_name(t);
  size_t len = strlen("<ptr >") + strlen(type_name);
  char *buf = (char *)allocator_alloc(vm_get_allocator(vm), len + 1);
  snprintf(buf, len + 1, "<ptr %s>", type_name);
  value_t result = create_str_value(vm, buf);
  allocator_free(vm_get_allocator(vm), &buf);
  return result;
}

/* ---- VTable: get_field / set_field (auto-deref) ---- */

static value_t _pointer_get_field(vm_t vm, value_t self, const char *name) {
  /* auto-deref: delegate to pointee's get_field */
  value_t derefed = _pointer_deref_get(vm, self);
  if (value_is_error(derefed))
    return derefed;
  vtable_t vt = type_get_vtable(value_get_type(derefed));
  if (!vt.get_field)
    return create_exception_value(vm, "type '%s' does not support field access",
                              type_get_name(value_get_type(derefed)));
  return vt.get_field(vm, derefed, name);
}

static value_t _pointer_set_field(vm_t vm, value_t self, const char *name, value_t val) {
  /* auto-deref: delegate to pointee's set_field */
  value_t derefed = _pointer_deref_get(vm, self);
  if (value_is_error(derefed))
    return derefed;
  vtable_t vt = type_get_vtable(value_get_type(derefed));
  if (!vt.set_field)
    return create_exception_value(vm, "type '%s' does not support field assignment",
                              type_get_name(value_get_type(derefed)));
  return vt.set_field(vm, derefed, name, val);
}

static value_t _pointer_get_field_raw(vm_t vm, value_t self, const char *name) {
  /* auto-deref: delegate to pointee's get_field_raw */
  value_t derefed = _pointer_deref_get(vm, self);
  if (value_is_error(derefed))
    return derefed;
  vtable_t vt = type_get_vtable(value_get_type(derefed));
  if (!vt.get_field_raw)
    return create_exception_value(vm, "type '%s' does not support raw field access",
                              type_get_name(value_get_type(derefed)));
  return vt.get_field_raw(vm, derefed, name);
}

/* ---- VTable: member_call / get_prop / set_prop (auto-deref) ---- */

static value_t _pointer_member_call(vm_t vm, value_t self, const char *name,
                                     size_t argc, value_t *argv) {
  value_t derefed = _pointer_deref_get(vm, self);
  if (value_is_error(derefed))
    return derefed;
  vtable_t vt = type_get_vtable(value_get_type(derefed));
  if (!vt.member_call)
    return create_exception_value(vm, "type '%s' does not support member call",
                              type_get_name(value_get_type(derefed)));
  return vt.member_call(vm, derefed, name, argc, argv);
}

/* ---- VTable: is_instance (auto-deref) ---- */

static value_t _pointer_is_instance(vm_t vm, value_t self, type_t type) {
  value_t derefed = _pointer_deref_get(vm, self);
  if (value_is_error(derefed))
    return derefed;
  vtable_t vt = type_get_vtable(value_get_type(derefed));
  if (!vt.is_instance)
    return create_exception_value(vm, "type '%s' does not support 'is' operator",
                              type_get_name(value_get_type(derefed)));
  return vt.is_instance(vm, derefed, type);
}

