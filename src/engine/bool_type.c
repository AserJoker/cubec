#include "engine/bool_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/error_type.h"
#include "engine/void_type.h"
#include "engine/type.h"
#include <stdbool.h>
#include <string.h>

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
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
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

static value_t _bool_band(vm_t vm, value_t a, value_t b) {
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, *(bool *)value_get_data(a) && *(bool *)value_get_data(b));
}

static value_t _bool_bor(vm_t vm, value_t a, value_t b) {
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, *(bool *)value_get_data(a) || *(bool *)value_get_data(b));
}

static value_t _bool_bxor(vm_t vm, value_t a, value_t b) {
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  bool va = *(bool *)value_get_data(a);
  bool vb = *(bool *)value_get_data(b);
  return create_bool_value(vm, va != vb);
}

static value_t _bool_bnot(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, !*(bool *)value_get_data(a));
}

static value_t _bool_lnot(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, !*(bool *)value_get_data(a));
}

static value_t _bool_safe_cast(vm_t vm, value_t self, type_t to) {
  if (to->kind != TYPE_KIND_BOOL)
    return create_error_value(vm, "cannot safe_cast bool to '%s'", to->name);
  /* bool → bool or bool → const bool: always safe */
  if (to == value_get_type(self))
    return self;
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, to, NULL, true);
  bool *copy = (bool *)allocator_alloc(vm_get_allocator(vm), sizeof(bool));
  *copy = *(bool *)value_get_data(self);
  return value_create(vm_get_allocator(vm), to, copy, true);
}

static value_t _const_bool_safe_cast(vm_t vm, value_t self, type_t to) {
  if (to->kind != TYPE_KIND_BOOL)
    return create_error_value(vm, "cannot safe_cast const bool to '%s'", to->name);
  /* const bool → bool: not allowed (const → mutable is not safe) */
  if (to->mut)
    return create_error_value(vm, "cannot safe_cast const bool to bool");
  /* const bool → const bool */
  if (to == value_get_type(self))
    return self;
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, to, NULL, true);
  bool *copy = (bool *)allocator_alloc(vm_get_allocator(vm), sizeof(bool));
  *copy = *(bool *)value_get_data(self);
  return value_create(vm_get_allocator(vm), to, copy, true);
}

static value_t _bool_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  type_t rt = value_get_type(rvalue);
  if (rt->kind != TYPE_KIND_BOOL)
    return create_error_value(vm, "cannot assign '%s' to '%s'", rt->name, lt->name);
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  memcpy(value_get_data(lvalue), value_get_data(rvalue), lt->size);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

type_t type_get_bool_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t bool_type = {
      .kind  = TYPE_KIND_BOOL,
      .name  = (char *)"bool",
      .size  = 1,
      .align = 1,
      .mut   = true,
      .vtable = {
          .clone = _bool_clone,
          .dispose = _bool_dispose,
          .equal = _bool_equal,
          .extends = NULL,
          .type_equal = _bool_type_equal,
          .type_extends = _bool_type_extends,
          .band = _bool_band,
          .bor = _bool_bor,
          .bxor = _bool_bxor,
          .bnot = _bool_bnot,
          .lnot = _bool_lnot,
          .safe_cast = _bool_safe_cast,
          .assignment = _bool_assignment,
      },
  };
  return &bool_type;
}

/* ---- Const bool type vtable ---- */

static value_t _const_bool_clone(allocator_t allocator, value_t self) {
  bool *src = (bool *)value_get_data(self);
  bool *copy = (bool *)allocator_alloc(allocator, sizeof(bool));
  *copy = *src;
  /* clone of const produces const copy; assignment (safe_cast) produces mutable */
  return value_create(allocator, value_get_type(self), copy, true);
}

static void _const_bool_dispose(allocator_t allocator, value_t self) {
  bool *d = (bool *)value_get_data(self);
  allocator_free(allocator, &d);
}

type_t type_get_const_bool_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t const_bool_type = {
      .kind  = TYPE_KIND_BOOL,
      .name  = (char *)"const bool",
      .size  = 1,
      .align = 1,
      .mut   = false,
      .vtable = {
          .clone = _const_bool_clone,
          .dispose = _const_bool_dispose,
          .equal = _bool_equal,
          .extends = NULL,
          .type_equal = _bool_type_equal,
          .type_extends = _bool_type_extends,
          .band = _bool_band,
          .bor = _bool_bor,
          .bxor = _bool_bxor,
          .bnot = _bool_bnot,
          .lnot = _bool_lnot,
          .safe_cast = _const_bool_safe_cast,
      },
  };
  return &const_bool_type;
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
