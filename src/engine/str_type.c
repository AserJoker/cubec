#include "engine/str_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/error_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>

/* Helper: read the string_t from a str value */
static string_t _str_read(value_t v) {
  return *(string_t *)value_get_data(v);
}

/* Helper: create a str value from a C string */
static value_t _str_value_create(vm_t vm, const char *val) {
  allocator_t alloc = vm_get_allocator(vm);
  string_t *data = (string_t *)allocator_alloc(alloc, sizeof(string_t));
  string_init_t si = {.str = val};
  *data = (string_t)allocator_create(alloc, &g_string_class, &si);
  type_t t = (type_t)value_get_data(vm_get_str_type(vm));
  value_t v = value_create(alloc, t, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->strings, *data);
    vec_push(scope->values, v);
  }
  return v;
}

/* Helper: create a str value from an already-owned string_t */
static value_t _str_value_from_owned(vm_t vm, string_t owned) {
  allocator_t alloc = vm_get_allocator(vm);
  string_t *data = (string_t *)allocator_alloc(alloc, sizeof(string_t));
  *data = owned;
  type_t t = (type_t)value_get_data(vm_get_str_type(vm));
  value_t v = value_create(alloc, t, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->strings, owned);
    vec_push(scope->values, v);
  }
  return v;
}

/* ---- str vtable ---- */

static value_t _str_clone(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  return _str_value_create(vm, string_get(_str_read(self)));
}

static value_t _str_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot compare str with different kind");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, strcmp(string_get(_str_read(a)),
                                      string_get(_str_read(b))) == 0);
}

static value_t _str_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_STR);
}

static value_t _str_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_STR);
}

static value_t _str_add(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot concatenate str with non-str");
  type_t result_type = (type_t)value_get_data(vm_get_str_type(vm));
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, result_type, NULL, true);
  allocator_t alloc = vm_get_allocator(vm);
  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);
  string_concat(result, string_get(_str_read(a)));
  string_concat(result, string_get(_str_read(b)));
  return _str_value_from_owned(vm, result);
}

static value_t _str_gt(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot compare str with different kind");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, strcmp(string_get(_str_read(a)),
                                      string_get(_str_read(b))) > 0);
}

static value_t _str_lt(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot compare str with different kind");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, strcmp(string_get(_str_read(a)),
                                      string_get(_str_read(b))) < 0);
}

static value_t _str_lnot(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, string_get_length(_str_read(a)) == 0);
}

static value_t _str_safe_cast(vm_t vm, value_t self, type_t to) {
  if (to->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot safe_cast str to '%s'", to->name);
  if (to == value_get_type(self)) return self;
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, to, NULL, true);
  allocator_t alloc = vm_get_allocator(vm);
  string_t *data = (string_t *)allocator_alloc(alloc, sizeof(string_t));
  string_init_t si = {.str = string_get(_str_read(self))};
  *data = (string_t)allocator_create(alloc, &g_string_class, &si);
  value_t v = value_create(alloc, to, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->strings, *data);
    vec_push(scope->values, v);
  }
  return v;
}

static value_t _const_str_safe_cast(vm_t vm, value_t self, type_t to) {
  if (to->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot safe_cast const str to '%s'", to->name);
  if (to->mut)
    return create_error_value(vm, "cannot safe_cast const str to str");
  if (to == value_get_type(self)) return self;
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, to, NULL, true);
  allocator_t alloc = vm_get_allocator(vm);
  string_t *data = (string_t *)allocator_alloc(alloc, sizeof(string_t));
  string_init_t si = {.str = string_get(_str_read(self))};
  *data = (string_t)allocator_create(alloc, &g_string_class, &si);
  value_t v = value_create(alloc, to, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->strings, *data);
    vec_push(scope->values, v);
  }
  return v;
}

static value_t _str_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t rt = value_get_type(rvalue);
  if (rt->kind != TYPE_KIND_STR)
    return create_error_value(vm, "cannot assign non-str to str");
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  /* Create new string_t in scope->strings; old one is managed by scope */
  allocator_t alloc = vm_get_allocator(vm);
  string_t new_str = (string_t)allocator_create(alloc, &g_string_class,
      &(string_init_t){.str = string_get(_str_read(rvalue))});
  string_t *slot = (string_t *)value_get_data(lvalue);
  *slot = new_str;
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->strings, new_str);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- Unsupported operators ---- */

static value_t _str_unsupported(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "str does not support this operator");
}

static value_t _str_unsupported_unary(vm_t vm, value_t a) {
  (void)a;
  return create_error_value(vm, "str does not support this operator");
}

/* ---- Static type singletons ---- */

type_t type_get_str_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t t = {
      .kind  = TYPE_KIND_STR,
      .name  = (char *)"str",
      .size  = sizeof(string_t),
      .align = _Alignof(string_t),
      .mut   = true,
      .vtable = {
          .clone        = _str_clone,
          .equal        = _str_equal,
          .extends      = NULL,
          .type_equal   = _str_type_equal,
          .type_extends = _str_type_extends,
          .band         = _str_unsupported,
          .bor          = _str_unsupported,
          .bxor         = _str_unsupported,
          .bnot         = _str_unsupported_unary,
          .lnot         = _str_lnot,
          .add          = _str_add,
          .sub          = _str_unsupported,
          .mul          = _str_unsupported,
          .div          = _str_unsupported,
          .mod          = _str_unsupported,
          .shl          = _str_unsupported,
          .shr          = _str_unsupported,
          .pos          = _str_unsupported_unary,
          .neg          = _str_unsupported_unary,
          .gt           = _str_gt,
          .lt           = _str_lt,
          .safe_cast    = _str_safe_cast,
          .assignment   = _str_assignment,
          .to_string    = NULL,
      },
  };
  return &t;
}

type_t type_get_const_str_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t t = {
      .kind  = TYPE_KIND_STR,
      .name  = (char *)"const str",
      .size  = sizeof(string_t),
      .align = _Alignof(string_t),
      .mut   = false,
      .vtable = {
          .clone        = _str_clone,
          .equal        = _str_equal,
          .extends      = NULL,
          .type_equal   = _str_type_equal,
          .type_extends = _str_type_extends,
          .band         = _str_unsupported,
          .bor          = _str_unsupported,
          .bxor         = _str_unsupported,
          .bnot         = _str_unsupported_unary,
          .lnot         = _str_lnot,
          .add          = _str_add,
          .sub          = _str_unsupported,
          .mul          = _str_unsupported,
          .div          = _str_unsupported,
          .mod          = _str_unsupported,
          .shl          = _str_unsupported,
          .shr          = _str_unsupported,
          .pos          = _str_unsupported_unary,
          .neg          = _str_unsupported_unary,
          .gt           = _str_gt,
          .lt           = _str_lt,
          .safe_cast    = _const_str_safe_cast,
          .assignment   = NULL,
          .to_string    = NULL,
      },
  };
  return &t;
}

value_t create_str_value(vm_t vm, const char *val) {
  return _str_value_create(vm, val);
}
