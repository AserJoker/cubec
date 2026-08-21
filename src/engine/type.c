#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "engine/bool_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/tuple_type.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

type_kind_t type_get_kind(type_t self) { return self->kind; }
const char *type_get_name(type_t self) { return self->name; }
uint64_t    type_get_size(type_t self) { return self->size; }
uint64_t    type_get_align(type_t self) { return self->align; }
bool        type_is_mut(type_t self) { return self->mut; }
void        type_set_mut(type_t self, bool mut) { self->mut = mut; }
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
    .clone   = NULL, /* types are global singletons — alloc_clone aborts */
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

type_t type_clone(vm_t vm, type_t self) {
  if (!self || !self->vtable.type_clone)
    return NULL;
  return self->vtable.type_clone(vm, self);
}

/* ---- type_create_with_mut: create a variant of a type with different mut ---- */

/**
 * @brief Look up a pre-created primitive type variant from vm.
 * For primitive types (bool, i8-i64, u8-u64, f16-f64, str, wildcard),
 * the vm already has both mutable and const variants pre-created.
 * @return the type_t if found, NULL if not a primitive type.
 */
static type_t _lookup_primitive_variant(vm_t vm, type_t src, bool mut) {
  if (type_is_mut(src) == mut)
    return src; /* already the requested mutability */

  switch (src->kind) {
    case TYPE_KIND_BOOL:
      return (type_t)value_get_data(mut ? vm_get_bool_type(vm)
                                        : vm_get_const_bool_type(vm));
    case TYPE_KIND_I8:
      return (type_t)value_get_data(mut ? vm_get_i8_type(vm)
                                        : vm_get_const_i8_type(vm));
    case TYPE_KIND_I16:
      return (type_t)value_get_data(mut ? vm_get_i16_type(vm)
                                        : vm_get_const_i16_type(vm));
    case TYPE_KIND_I32:
      return (type_t)value_get_data(mut ? vm_get_i32_type(vm)
                                        : vm_get_const_i32_type(vm));
    case TYPE_KIND_I64:
      return (type_t)value_get_data(mut ? vm_get_i64_type(vm)
                                        : vm_get_const_i64_type(vm));
    case TYPE_KIND_U8:
      return (type_t)value_get_data(mut ? vm_get_u8_type(vm)
                                        : vm_get_const_u8_type(vm));
    case TYPE_KIND_U16:
      return (type_t)value_get_data(mut ? vm_get_u16_type(vm)
                                        : vm_get_const_u16_type(vm));
    case TYPE_KIND_U32:
      return (type_t)value_get_data(mut ? vm_get_u32_type(vm)
                                        : vm_get_const_u32_type(vm));
    case TYPE_KIND_U64:
      return (type_t)value_get_data(mut ? vm_get_u64_type(vm)
                                        : vm_get_const_u64_type(vm));
    case TYPE_KIND_F16:
      return (type_t)value_get_data(mut ? vm_get_f16_type(vm)
                                        : vm_get_const_f16_type(vm));
    case TYPE_KIND_F32:
      return (type_t)value_get_data(mut ? vm_get_f32_type(vm)
                                        : vm_get_const_f32_type(vm));
    case TYPE_KIND_F64:
      return (type_t)value_get_data(mut ? vm_get_f64_type(vm)
                                        : vm_get_const_f64_type(vm));
    case TYPE_KIND_STR:
      return (type_t)value_get_data(mut ? vm_get_str_type(vm)
                                        : vm_get_const_str_type(vm));
    case TYPE_KIND_WILDCARD:
      return (type_t)value_get_data(mut ? vm_get_wildcard_type(vm)
                                        : vm_get_const_wildcard_type(vm));
    default:
      return NULL; /* not a primitive type — caller must create a new one */
  }
}

/**
 * @brief Create a new type that is identical to src but with the specified
 * mut flag. For primitive types, returns the vm's pre-created variant.
 * For composite types, creates a new type_t with borrowed sub-type pointers.
 *
 * The returned type_t is registered in vm->types.
 */
type_t type_create_with_mut(vm_t vm, type_t src, bool mut) {
  /* 1. same mutability — return as-is */
  if (type_is_mut(src) == mut)
    return src;

  /* 2. primitive types: use vm's pre-created variants */
  type_t prim = _lookup_primitive_variant(vm, src, mut);
  if (prim)
    return prim;

  /* 3. composite types: type_clone + set mut */
  type_t cloned = type_clone(vm, src);
  if (!cloned)
    return NULL;
  cloned->mut = mut;
  return cloned;
}

/* ---- Bootstrap type "type" vtable ---- */

static value_t _type_clone(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, value_is_initialized(self));
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  type_t inner = (type_t)value_get_data(self);
  allocator_t allocator = vm_get_allocator(vm);
  /* types are global singletons managed by vm->types — share the same pointer.
   * own=false: value does not own the type_t lifecycle. */
  value_t v = value_create(allocator, type_type, inner, false);
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
    return create_exception_value(vm, "cannot compare types of different kinds: %s vs %s",
                              ta->name, tb->name);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_bool_type(vm)), NULL, true);
  if (!ta->vtable.type_equal)
    return create_exception_value(vm, "type '%s' does not support type_equal", ta->name);
  return ta->vtable.type_equal(vm, ta, tb);
}

static value_t _type_extends(vm_t vm, value_t sub, value_t super_val) {
  type_t t_sub = (type_t)value_get_data(sub);
  type_t t_super = (type_t)value_get_data(super_val);
  /* wildcard short-circuit: any type extends wildcard */
  if (t_super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (t_sub->kind != t_super->kind)
    return create_exception_value(vm, "cannot check extends between types of different kinds: %s vs %s",
                              t_sub->name, t_super->name);
  if (value_is_shadow(sub) || value_is_shadow(super_val))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_bool_type(vm)), NULL, true);
  if (!t_sub->vtable.type_extends)
    return create_exception_value(vm, "type '%s' does not support type_extends", t_sub->name);
  return t_sub->vtable.type_extends(vm, t_sub, t_super);
}

static value_t _type_get_prop(vm_t vm, value_t self, const char *name) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  type_t inner = (type_t)value_get_data(self);
  if (!inner->vtable.type_get_prop)
    return create_exception_value(vm, "type '%s' does not support static property access",
                              inner->name);
  return inner->vtable.type_get_prop(vm, inner, name);
}

static value_t _type_set_prop(vm_t vm, value_t self, const char *name, value_t val) {
  if (value_is_shadow(self))
    return create_void_value(vm);
  type_t inner = (type_t)value_get_data(self);
  if (!inner->vtable.type_set_prop)
    return create_exception_value(vm, "type '%s' does not support static property assignment",
                              inner->name);
  return inner->vtable.type_set_prop(vm, inner, name, val);
}

static vec_t _type_spread(vm_t vm, value_t self) {
  type_t inner = (type_t)value_get_data(self);
  /* If the inner type is a tuple, expand it into individual type values */
  if (inner->kind == TYPE_KIND_TUPLE) {
    tuple_type_t tt = (tuple_type_t)inner;
    allocator_t allocator = vm_get_allocator(vm);
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    vec_init_t vi = {.auto_dispose = false};
    vec_t result = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
    uint64_t fc = tuple_type_get_field_count(tt);
    for (uint64_t i = 0; i < fc; i++) {
      type_t elem = tuple_type_get_element_type(tt, i);
      /* wrap each element type as a type value */
      value_t tv = vm_create_value_ref(vm, type_type, elem, NULL);
      vec_push(result, tv);
    }
    return result;
  }
  return NULL;
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
          .spread = _type_spread,
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
