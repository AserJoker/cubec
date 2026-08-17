#include "engine/opaque_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdio.h>

/* ---- Opaque type vtable ---- */

static value_t _opaque_clone(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_opaque_type(vm)),
                                  NULL, true);
  void **src = (void **)value_get_data(self);
  return create_opaque_value(vm, src ? *src : NULL);
}

static value_t _opaque_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);

  /* opaque == opaque */
  if (tb->kind == TYPE_KIND_OPAQUE) {
    if (value_is_shadow(a) || value_is_shadow(b))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    void **pa = (void **)value_get_data(a);
    void **pb = (void **)value_get_data(b);
    return create_bool_value(vm, (pa ? *pa : NULL) == (pb ? *pb : NULL));
  }

  /* opaque == nil: true if opaque holds NULL */
  if (tb->kind == TYPE_KIND_NIL) {
    if (value_is_shadow(a) || value_is_shadow(b))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    void **pa = (void **)value_get_data(a);
    return create_bool_value(vm, pa == NULL || *pa == NULL);
  }

  /* opaque vs incompatible type */
  return create_exception_value(vm, "cannot compare opaque with '%s'",
                                type_get_name(tb));
}

static value_t _opaque_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_OPAQUE);
}

static value_t _opaque_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_OPAQUE);
}

static value_t _opaque_safe_cast(vm_t vm, value_t self, type_t to) {
  /* opaque -> opaque: identity */
  if (to->kind == TYPE_KIND_OPAQUE)
    return self;

  /* wildcard accepts anything */
  if (to->kind == TYPE_KIND_WILDCARD)
    return self;

  return create_exception_value(vm, "cannot safe_cast opaque to '%s'", to->name);
}

static value_t _opaque_to_string(vm_t vm, value_t self) {
  void **ptr = (void **)value_get_data(self);
  char buf[32];
  snprintf(buf, sizeof(buf), "opaque(%p)", ptr ? *ptr : NULL);
  return create_str_value(vm, buf);
}

type_t type_get_opaque_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_OPAQUE,
      .name  = "opaque",
      .size  = sizeof(void *),
      .align = _Alignof(void *),
      .mut   = false,
      .vtable = {
          .clone        = _opaque_clone,
          .equal        = _opaque_equal,
          .type_equal   = _opaque_type_equal,
          .type_extends = _opaque_type_extends,
          .safe_cast    = _opaque_safe_cast,
          .to_string    = _opaque_to_string,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

value_t create_opaque_value(vm_t vm, void *addr) {
  allocator_t allocator = vm_get_allocator(vm);
  type_t opaque_type = (type_t)value_get_data(vm_get_opaque_type(vm));
  void **data = (void **)allocator_alloc(allocator, sizeof(void *));
  *data = addr;
  value_t v = value_create(allocator, opaque_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope)
    vec_push(scope->values, v);
  return v;
}
