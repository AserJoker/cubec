#include "engine/nil_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "engine/pointer_type.h"
#include "engine/opaque_type.h"
#include "core/string.h"

/* ---- Nil type vtable ---- */

static value_t _nil_clone(vm_t vm, value_t self) {
  (void)self;
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_nil_type(vm)),
                                  NULL, true);
  return create_nil_value(vm);
}

static value_t _nil_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);

  /* nil == nil */
  if (tb->kind == TYPE_KIND_NIL) {
    if (value_is_shadow(a) || value_is_shadow(b))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    return create_bool_value(vm, true);
  }

  /* nil == pointer: true if pointer holds NULL address */
  if (tb->kind == TYPE_KIND_POINTER) {
    if (value_is_shadow(a) || value_is_shadow(b))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    void **pb = (void **)value_get_data(b);
    return create_bool_value(vm, pb == NULL || *pb == NULL);
  }

  /* nil == opaque: true if opaque holds NULL address */
  if (tb->kind == TYPE_KIND_OPAQUE) {
    if (value_is_shadow(a) || value_is_shadow(b))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    void **pb = (void **)value_get_data(b);
    return create_bool_value(vm, pb == NULL || *pb == NULL);
  }

  /* nil vs incompatible type */
  return create_exception_value(vm, "cannot compare nil with '%s'",
                                type_get_name(tb));
}

static value_t _nil_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_NIL);
}

static value_t _nil_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_NIL);
}

static value_t _nil_safe_cast(vm_t vm, value_t self, type_t to) {
  /* nil -> nil: identity */
  if (to->kind == TYPE_KIND_NIL)
    return self;

  /* nil -> pointer: create null pointer of target type */
  if (to->kind == TYPE_KIND_POINTER) {
    if (value_is_shadow(self))
      return vm_create_value_shadow(vm, to, NULL, true);
    type_t cloned_type = to;
    pointer_type_t dst_pt = (pointer_type_t)cloned_type;
    return create_pointer_value_from_addr(vm, dst_pt, NULL);
  }

  /* nil -> opaque: create null opaque */
  if (to->kind == TYPE_KIND_OPAQUE) {
    if (value_is_shadow(self))
      return vm_create_value_shadow(vm, to, NULL, true);
    return create_opaque_value(vm, NULL);
  }

  /* wildcard accepts anything */
  if (to->kind == TYPE_KIND_WILDCARD)
    return self;

  return create_exception_value(vm, "cannot safe_cast nil to '%s'", to->name);
}

static value_t _nil_to_string(vm_t vm, value_t self) {
  (void)self;
  return create_str_value(vm, "nil");
}

type_t type_get_nil_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_NIL,
      .name  = "nil",
      .size  = sizeof(void *),
      .align = _Alignof(void *),
      .mut   = false,
      .vtable = {
          .clone        = _nil_clone,
          .equal        = _nil_equal,
          .type_equal   = _nil_type_equal,
          .type_extends = _nil_type_extends,
          .safe_cast    = _nil_safe_cast,
          .to_string    = _nil_to_string,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

value_t create_nil_value(vm_t vm) {
  allocator_t allocator = vm_get_allocator(vm);
  type_t nil_type = (type_t)value_get_data(vm_get_nil_type(vm));
  /* nil stores a void* sized buffer holding NULL; must not pass data=NULL
   * or the value would be indistinguishable from a shadow value */
  void **data = (void **)allocator_alloc(allocator, sizeof(void *));
  *data = NULL;
  value_t v = value_create(allocator, nil_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope)
    vec_push(scope->values, v);
  return v;
}
