#include "engine/semantic_type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ===== type_name_entry lifecycle ===== */

static void _type_name_entry_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  semantic_type_t t = (semantic_type_t)self;
  const char *name = (const char *)arg;
  memset(t, 0, sizeof(struct type_name_entry));
  t->name = name;
  t->is_incomplete = true;

  vec_init_t vec_init = {.auto_dispose = false};
  t->instance_methods = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  t->static_methods = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  t->static_fields = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  t->associated_types = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
}

static void _type_name_entry_dispose(void *self, allocator_t allocator) {
  semantic_type_t t = (semantic_type_t)self;
  allocator_free(allocator, &t->instance_methods);
  allocator_free(allocator, &t->static_methods);
  allocator_free(allocator, &t->static_fields);
  allocator_free(allocator, &t->associated_types);
  allocator_free(allocator, &t->impl);
}

type_t g_semantic_type_type = {
    .size = sizeof(struct type_name_entry),
    .name = "cubec.engine.semantic_type",
    .init = (type_init_fn_t)_type_name_entry_init,
    .dispose = (type_dispose_fn_t)_type_name_entry_dispose,
};

/* ===== type_impl lifecycle ===== */

static void _type_impl_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  (void)arg;
  type_impl_t impl = (type_impl_t)self;
  memset(impl, 0, sizeof(struct _type_impl));
}

static void _type_impl_dispose(void *self, allocator_t allocator) {
  type_impl_t impl = (type_impl_t)self;
  /* Dispose owned vec_t fields depending on kind */
  switch (impl->kind) {
  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION:
    if (impl->struct_type.fields) {
      allocator_free(allocator, &impl->struct_type.fields);
    }
    break;
  case TYPE_ENUM:
    if (impl->enum_type.items) {
      allocator_free(allocator, &impl->enum_type.items);
    }
    break;
  case TYPE_INTERFACE:
    if (impl->interface_type.methods) {
      allocator_free(allocator, &impl->interface_type.methods);
    }
    break;
  case TYPE_FUNCTION:
    if (impl->function.params) {
      allocator_free(allocator, &impl->function.params);
    }
    break;
  default:
    break;
  }
}

type_t g_type_impl_type = {
    .size = sizeof(struct _type_impl),
    .name = "cubec.engine.type_impl",
    .init = (type_init_fn_t)_type_impl_init,
    .dispose = (type_dispose_fn_t)_type_impl_dispose,
};

/* ===== query API ===== */

enum type_kind semantic_type_get_kind(semantic_type_t self) {
  return self->impl ? self->impl->kind : TYPE_VOID;
}

const char *semantic_type_get_name(semantic_type_t self) {
  return self->name;
}

size_t semantic_type_get_size(semantic_type_t self) {
  return self->impl ? self->impl->size : 0;
}

size_t semantic_type_get_alignment(semantic_type_t self) {
  return self->impl ? self->impl->alignment : 0;
}

bool semantic_type_is_incomplete(semantic_type_t self) {
  return self->is_incomplete;
}

type_impl_t semantic_type_get_impl(semantic_type_t self) {
  return self->impl;
}

/* ===== structural equality ===== */

static bool _type_impl_equals(type_impl_t a, type_impl_t b);

static bool _type_vec_equals(vec_t a, vec_t b) {
  size_t sa = vec_get_size(a);
  size_t sb = vec_get_size(b);
  if (sa != sb) return false;
  for (size_t i = 0; i < sa; i++) {
    semantic_type_t ta = (semantic_type_t)vec_get(a, i);
    semantic_type_t tb = (semantic_type_t)vec_get(b, i);
    if (!semantic_type_equals(ta, tb)) return false;
  }
  return true;
}

static bool _type_impl_equals(type_impl_t a, type_impl_t b) {
  if (a == b) return true;
  if (!a || !b) return false;
  if (a->kind != b->kind) return false;
  if (a->hash != 0 && b->hash != 0 && a->hash != b->hash) return false;

  switch (a->kind) {
  case TYPE_VOID: case TYPE_BOOL:
  case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
  case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
  case TYPE_F16: case TYPE_F32: case TYPE_F64:
  case TYPE_CHAR: case TYPE_STRING:
  case TYPE_NIL: case TYPE_ERROR:
    return true;

  case TYPE_QUALIFIER:
    return semantic_type_equals(a->qualifier.base, b->qualifier.base) &&
           a->qualifier.is_volatile == b->qualifier.is_volatile;

  case TYPE_POINTER:
    return semantic_type_equals(a->pointer.pointee, b->pointer.pointee);

  case TYPE_SLICE:
    return semantic_type_equals(a->slice.element, b->slice.element);

  case TYPE_ARRAY:
    return a->array.length == b->array.length &&
           semantic_type_equals(a->array.element, b->array.element);

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION:
    return _type_vec_equals(a->struct_type.fields, b->struct_type.fields);

  case TYPE_ENUM:
    return semantic_type_equals(a->enum_type.backing_type,
                                b->enum_type.backing_type);

  case TYPE_INTERFACE:
    return _type_vec_equals(a->interface_type.methods,
                            b->interface_type.methods);

  case TYPE_FUNCTION:
    return semantic_type_equals(a->function.return_type,
                                b->function.return_type) &&
           a->function.is_variadic == b->function.is_variadic &&
           _type_vec_equals(a->function.params, b->function.params);

  case TYPE_TYPE:
    return semantic_type_equals(a->type_of.inner, b->type_of.inner);

  default:
    return false;
  }
}

bool semantic_type_equals(semantic_type_t a, semantic_type_t b) {
  if (a == b) return true;
  if (!a || !b) return false;
  /* Same name => same type (nominal equality for named types) */
  if (a->name && b->name && strcmp(a->name, b->name) == 0) return true;
  /* Structural equality via impl */
  return _type_impl_equals(a->impl, b->impl);
}

/* ===== decay and implicit conversion ===== */

bool semantic_type_can_decay(semantic_type_t from, semantic_type_t to) {
  if (!from || !to || !from->impl || !to->impl) return false;

  /* array -> slice */
  if (from->impl->kind == TYPE_ARRAY && to->impl->kind == TYPE_SLICE) {
    return semantic_type_equals(from->impl->array.element,
                                to->impl->slice.element);
  }
  /* array -> pointer */
  if (from->impl->kind == TYPE_ARRAY && to->impl->kind == TYPE_POINTER) {
    return semantic_type_equals(from->impl->array.element,
                                to->impl->pointer.pointee);
  }
  /* slice -> pointer (deprecated but allowed) */
  if (from->impl->kind == TYPE_SLICE && to->impl->kind == TYPE_POINTER) {
    return semantic_type_equals(from->impl->slice.element,
                                to->impl->pointer.pointee);
  }
  return false;
}

bool semantic_type_can_implicit_convert(semantic_type_t from,
                                        semantic_type_t to) {
  if (semantic_type_equals(from, to)) return true;
  if (!from || !to || !from->impl || !to->impl) return false;

  /* nil -> pointer/slice/interface */
  if (from->impl->kind == TYPE_NIL) {
    return to->impl->kind == TYPE_POINTER ||
           to->impl->kind == TYPE_SLICE ||
           to->impl->kind == TYPE_INTERFACE;
  }

  /* integer widening */
  if (from->impl->kind >= TYPE_I8 && from->impl->kind <= TYPE_U64 &&
      to->impl->kind >= TYPE_I8 && to->impl->kind <= TYPE_U64) {
    return to->impl->size >= from->impl->size;
  }

  /* float widening */
  if (from->impl->kind >= TYPE_F16 && from->impl->kind <= TYPE_F64 &&
      to->impl->kind >= TYPE_F16 && to->impl->kind <= TYPE_F64) {
    return to->impl->size >= from->impl->size;
  }

  /* int -> float */
  if (from->impl->kind >= TYPE_I8 && from->impl->kind <= TYPE_U64 &&
      to->impl->kind >= TYPE_F16 && to->impl->kind <= TYPE_F64) {
    return true;
  }

  /* decay */
  return semantic_type_can_decay(from, to);
}

/* ===== constructors ===== */

static type_impl_t _create_impl(allocator_t allocator, enum type_kind kind) {
  type_impl_t impl =
      (type_impl_t)allocator_create(allocator, &g_type_impl_type, NULL);
  impl->kind = kind;
  return impl;
}

semantic_type_t semantic_type_create_named(allocator_t allocator,
                                           const char *name,
                                           enum type_kind kind) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, (void *)name);
  t->impl = _create_impl(allocator, kind);
  /* Primitive types are complete immediately */
  if (kind >= TYPE_VOID && kind <= TYPE_STRING) {
    t->is_incomplete = false;
  }
  return t;
}

semantic_type_t semantic_type_create_pointer(allocator_t allocator,
                                             semantic_type_t pointee) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_POINTER);
  t->impl->pointer.pointee = pointee;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_slice(allocator_t allocator,
                                           semantic_type_t element) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_SLICE);
  t->impl->slice.element = element;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_array(allocator_t allocator,
                                           semantic_type_t element,
                                           size_t length) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_ARRAY);
  t->impl->array.element = element;
  t->impl->array.length = length;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_qualifier(allocator_t allocator,
                                               semantic_type_t base,
                                               bool is_volatile) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_QUALIFIER);
  t->impl->qualifier.base = base;
  t->impl->qualifier.is_volatile = is_volatile;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_function(allocator_t allocator,
                                              semantic_type_t return_type,
                                              vec_t params,
                                              bool is_variadic) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_FUNCTION);
  t->impl->function.return_type = return_type;
  t->impl->function.params = params;
  t->impl->function.is_variadic = is_variadic;
  t->is_incomplete = false;
  return t;
}
