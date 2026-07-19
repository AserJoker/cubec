#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "engine/symbol.h"
#include "engine/comptime_value.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <stdio.h>
#include <string.h>

/* ===== type_name_entry lifecycle ===== */

static void _type_name_entry_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  semantic_type_t t = (semantic_type_t)self;
  const char *name = (const char *)arg;
  memset(t, 0, sizeof(struct type_name_entry));
  t->name = name;
  t->is_incomplete = true;

  vec_init_t vec_init = {.auto_dispose = true};
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
  case TYPE_GENERIC_INSTANCE:
    if (impl->generic_instance.type_args) {
      allocator_free(allocator, &impl->generic_instance.type_args);
    }
    if (impl->generic_instance.fields) {
      allocator_free(allocator, &impl->generic_instance.fields);
    }
    break;
  case TYPE_GENERIC_PACK:
    if (impl->generic_pack.expanded_types) {
      allocator_free(allocator, &impl->generic_pack.expanded_types);
    }
    break;
  case TYPE_TUPLE:
    if (impl->tuple.element_types) {
      allocator_free(allocator, &impl->tuple.element_types);
    }
    if (impl->tuple.fields) {
      allocator_free(allocator, &impl->tuple.fields);
    }
    break;
  case TYPE_GENERIC_VALUE:
    if (impl->generic_value.value) {
      allocator_free(allocator, &impl->generic_value.value);
    }
    break;
  case TYPE_PACK_INDEX:
    /* pack_name and index_type are not owned */
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
  case TYPE_WILDCARD:
    return true;

  case TYPE_QUALIFIER:
    return semantic_type_equals(a->qualifier.base, b->qualifier.base) &&
           a->qualifier.is_const == b->qualifier.is_const &&
           a->qualifier.is_volatile == b->qualifier.is_volatile;

  case TYPE_POINTER:
    return semantic_type_equals(a->pointer.pointee, b->pointer.pointee);

  case TYPE_SLICE:
    return semantic_type_equals(a->slice.element, b->slice.element);

  case TYPE_ARRAY:
    if (a->array.length_param_idx != b->array.length_param_idx) return false;
    if (a->array.length_param_idx == (size_t)-1 &&
        a->array.length != b->array.length) return false;
    return semantic_type_equals(a->array.element, b->array.element);

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

  case TYPE_GENERIC_INSTANCE:
    return semantic_type_equals(a->generic_instance.generic_template,
                                b->generic_instance.generic_template) &&
           _type_vec_equals(a->generic_instance.type_args,
                            b->generic_instance.type_args);

  case TYPE_GENERIC_PARAM:
    if (a->generic_param.index != b->generic_param.index) return false;
    if (a->generic_param.is_value != b->generic_param.is_value) return false;
    if (a->generic_param.name && b->generic_param.name
            ? strcmp(a->generic_param.name, b->generic_param.name) != 0
            : a->generic_param.name != b->generic_param.name) return false;
    if (a->generic_param.value_type || b->generic_param.value_type) {
      if (!a->generic_param.value_type || !b->generic_param.value_type) return false;
      if (!semantic_type_equals(a->generic_param.value_type,
                                b->generic_param.value_type)) return false;
    }
    return true;

  case TYPE_GENERIC_PACK:
    if (a->generic_pack.index != b->generic_pack.index) return false;
    if (a->generic_pack.name && b->generic_pack.name &&
        strcmp(a->generic_pack.name, b->generic_pack.name) != 0) return false;
    return _type_vec_equals(a->generic_pack.expanded_types,
                            b->generic_pack.expanded_types);

  case TYPE_TUPLE:
    return _type_vec_equals(a->tuple.element_types,
                            b->tuple.element_types);

  case TYPE_OPAQUE:
    return true;

  case TYPE_PACK_INDEX:
    if (a->pack_index.pack_param_idx != b->pack_index.pack_param_idx) return false;
    if (a->pack_index.index_param_idx != b->pack_index.index_param_idx) return false;
    if (a->pack_index.pack_name && b->pack_index.pack_name &&
        strcmp(a->pack_index.pack_name, b->pack_index.pack_name) != 0) return false;
    return true;

  case TYPE_GENERIC_VALUE: {
    comptime_value_t av = a->generic_value.value;
    comptime_value_t bv = b->generic_value.value;
    if (!av || !bv) return av == bv;
    if (av->kind != bv->kind) return false;
    switch (av->kind) {
    case COMPTIME_VALUE_INT:
      return av->int_val.u == bv->int_val.u &&
             av->int_val.is_signed == bv->int_val.is_signed &&
             av->int_val.width == bv->int_val.width;
    case COMPTIME_VALUE_BOOL:
      return av->bool_val == bv->bool_val;
    case COMPTIME_VALUE_STRING:
      return strcmp(string_get(av->string_val), string_get(bv->string_val)) == 0;
    case COMPTIME_VALUE_CHAR:
      return av->char_val == bv->char_val;
    case COMPTIME_VALUE_FLOAT:
      return av->float_val.value == bv->float_val.value;
    default:
      return false;
    }
  }

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

  /* Adding qualifiers is safe: T → const T, T → volatile T, T → const volatile T */
  if (to->impl->kind == TYPE_QUALIFIER) {
    semantic_type_t to_base = semantic_type_strip_qualifier(to);
    if (semantic_type_can_implicit_convert(from, to_base)) return true;
  }

  /* Pointer conversion: only qualifier addition (*T → *const T) is implicit.
     Pointee types must be structurally equivalent (same type or qualifier addition).
     *Small → *Big downcast and other pointer conversions require explicit cast. */
  {
    semantic_type_t from_unq = semantic_type_strip_qualifier(from);
    semantic_type_t to_unq = semantic_type_strip_qualifier(to);
    if (from_unq->impl->kind == TYPE_POINTER && to_unq->impl->kind == TYPE_POINTER) {
      semantic_type_t from_pt = from_unq->impl->pointer.pointee;
      semantic_type_t to_pt = to_unq->impl->pointer.pointee;
      /* Only allow if pointee types are the same (after stripping qualifiers) */
      semantic_type_t from_pt_unq = semantic_type_strip_qualifier(from_pt);
      semantic_type_t to_pt_unq = semantic_type_strip_qualifier(to_pt);
      if (semantic_type_equals(from_pt_unq, to_pt_unq))
        return true;
    }
  }

  /* nil -> pointer/slice/interface */
  if (from->impl->kind == TYPE_NIL) {
    semantic_type_t to_unq = semantic_type_strip_qualifier(to);
    return to_unq->impl->kind == TYPE_POINTER ||
           to_unq->impl->kind == TYPE_SLICE ||
           to_unq->impl->kind == TYPE_INTERFACE;
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

  /* tuple element-wise implicit conversion */
  if (from->impl->kind == TYPE_TUPLE && to->impl->kind == TYPE_TUPLE) {
    vec_t from_elems = from->impl->tuple.element_types;
    vec_t to_elems = to->impl->tuple.element_types;
    size_t fc = vec_get_size(from_elems);
    size_t tc = vec_get_size(to_elems);
    if (fc != tc) return false;
    for (size_t i = 0; i < fc; i++) {
      semantic_type_t fe = (semantic_type_t)vec_get(from_elems, i);
      semantic_type_t te = (semantic_type_t)vec_get(to_elems, i);
      if (!semantic_type_can_implicit_convert(fe, te)) return false;
    }
    return true;
  }

  /* tuple → array: layout-compatible (element size + alignment must match) */
  if (from->impl->kind == TYPE_TUPLE && to->impl->kind == TYPE_ARRAY) {
    vec_t from_elems = from->impl->tuple.element_types;
    size_t fc = vec_get_size(from_elems);
    if (fc != to->impl->array.length) return false;
    semantic_type_t elem_type = to->impl->array.element;
    for (size_t i = 0; i < fc; i++) {
      semantic_type_t fe = (semantic_type_t)vec_get(from_elems, i);
      if (semantic_type_get_size(fe) != semantic_type_get_size(elem_type) ||
          semantic_type_get_alignment(fe) != semantic_type_get_alignment(elem_type))
        return false;
    }
    return true;
  }

  /* any pointer/slice → opaque */
  {
    semantic_type_t to_unq = semantic_type_strip_qualifier(to);
    if (to_unq->impl->kind == TYPE_OPAQUE) {
      semantic_type_t from_unq = semantic_type_strip_qualifier(from);
      return from_unq->impl->kind == TYPE_POINTER ||
             from_unq->impl->kind == TYPE_SLICE;
    }
  }

  /* struct-like to struct-like: field-wise implicit conversion (anonymous → named/generic_instance) */
  {
    semantic_type_t from_unq = semantic_type_strip_qualifier(from);
    semantic_type_t to_unq = semantic_type_strip_qualifier(to);
    bool from_struct = from_unq->impl->kind == TYPE_STRUCT ||
                       from_unq->impl->kind == TYPE_UNION ||
                       from_unq->impl->kind == TYPE_CUNION;
    bool to_struct = to_unq->impl->kind == TYPE_STRUCT ||
                     to_unq->impl->kind == TYPE_UNION ||
                     to_unq->impl->kind == TYPE_CUNION ||
                     to_unq->impl->kind == TYPE_GENERIC_INSTANCE;
    if (from_struct && to_struct) {
      vec_t from_fields = from_unq->impl->kind == TYPE_GENERIC_INSTANCE
          ? from_unq->impl->generic_instance.fields
          : from_unq->impl->struct_type.fields;
      vec_t to_fields = to_unq->impl->kind == TYPE_GENERIC_INSTANCE
          ? to_unq->impl->generic_instance.fields
          : to_unq->impl->struct_type.fields;
      size_t fc = from_fields ? vec_get_size(from_fields) : 0;
      size_t tc = to_fields ? vec_get_size(to_fields) : 0;
      if (fc != tc) return false;
      for (size_t i = 0; i < fc; i++) {
        struct symbol *ff = (struct symbol *)vec_get(from_fields, i);
        struct symbol *tf = (struct symbol *)vec_get(to_fields, i);
        if (!ff || !tf) return false;
        if (!ff->name || !tf->name || strcmp(ff->name, tf->name) != 0)
          return false;
        if (!semantic_type_can_implicit_convert(ff->field.type, tf->field.type))
          return false;
      }
      return true;
    }
  }

  /* decay */
  return semantic_type_can_decay(from, to);
}

/* ===== explicit cast ===== */

static bool _explicit_cast_numeric(semantic_type_t from, semantic_type_t to) {
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);
  enum type_kind fk = from_unq->impl->kind;
  enum type_kind tk = to_unq->impl->kind;

  /* float → int */
  if (fk >= TYPE_F16 && fk <= TYPE_F64 && tk >= TYPE_I8 && tk <= TYPE_U64)
    return true;
  /* int → int (narrowing or same-width) */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk >= TYPE_I8 && tk <= TYPE_U64)
    return true;
  /* float → float (narrowing) */
  if (fk >= TYPE_F16 && fk <= TYPE_F64 && tk >= TYPE_F16 && tk <= TYPE_F64)
    return true;
  /* bool → int */
  if (fk == TYPE_BOOL && tk >= TYPE_I8 && tk <= TYPE_U64)
    return true;
  /* int → bool */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_BOOL)
    return true;
  /* enum → int */
  if (fk == TYPE_ENUM && tk >= TYPE_I8 && tk <= TYPE_U64)
    return true;
  /* int → enum */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_ENUM)
    return true;
  /* char → int */
  if (fk == TYPE_CHAR && tk >= TYPE_I8 && tk <= TYPE_U64)
    return true;
  /* int → char */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_CHAR)
    return true;

  return false;
}

static bool _explicit_cast_pointer(semantic_type_t from, semantic_type_t to) {
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);

  /* opaque → pointer */
  if (from_unq->impl->kind == TYPE_OPAQUE && to_unq->impl->kind == TYPE_POINTER)
    return true;
  /* pointer → int */
  if (from_unq->impl->kind == TYPE_POINTER &&
      to_unq->impl->kind >= TYPE_I8 && to_unq->impl->kind <= TYPE_U64)
    return true;
  /* *Small → *Big (struct pointer downcast) */
  if (from_unq->impl->kind == TYPE_POINTER && to_unq->impl->kind == TYPE_POINTER) {
    semantic_type_t from_pt = semantic_type_strip_qualifier(from_unq->impl->pointer.pointee);
    semantic_type_t to_pt = semantic_type_strip_qualifier(to_unq->impl->pointer.pointee);
    vec_t from_fields = NULL;
    vec_t to_fields = NULL;
    if (from_pt->impl->kind == TYPE_STRUCT || from_pt->impl->kind == TYPE_UNION ||
        from_pt->impl->kind == TYPE_CUNION)
      from_fields = from_pt->impl->struct_type.fields;
    else if (from_pt->impl->kind == TYPE_GENERIC_INSTANCE)
      from_fields = from_pt->impl->generic_instance.fields;
    if (to_pt->impl->kind == TYPE_STRUCT || to_pt->impl->kind == TYPE_UNION ||
        to_pt->impl->kind == TYPE_CUNION)
      to_fields = to_pt->impl->struct_type.fields;
    else if (to_pt->impl->kind == TYPE_GENERIC_INSTANCE)
      to_fields = to_pt->impl->generic_instance.fields;

    if (from_fields && to_fields) {
      size_t fc = vec_get_size(from_fields);
      size_t tc = vec_get_size(to_fields);
      if (fc <= tc) {
        bool prefix_ok = true;
        for (size_t i = 0; i < fc && prefix_ok; i++) {
          struct symbol *ff = (struct symbol *)vec_get(from_fields, i);
          struct symbol *tf = (struct symbol *)vec_get(to_fields, i);
          if (!ff || !tf) prefix_ok = false;
          else if (!ff->name || !tf->name || strcmp(ff->name, tf->name) != 0)
            prefix_ok = false;
          else if (!semantic_type_equals(ff->field.type, tf->field.type))
            prefix_ok = false;
        }
        if (prefix_ok) return true;
      }
    }
  }

  return false;
}

static bool _explicit_cast_container(semantic_type_t from, semantic_type_t to) {
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);

  /* array → tuple (layout-compatible) */
  if (from_unq->impl->kind == TYPE_ARRAY && to_unq->impl->kind == TYPE_TUPLE) {
    size_t arr_len = from_unq->impl->array.length;
    vec_t to_elems = to_unq->impl->tuple.element_types;
    size_t tup_len = vec_get_size(to_elems);
    if (arr_len != tup_len) return false;
    semantic_type_t elem = from_unq->impl->array.element;
    for (size_t i = 0; i < tup_len; i++) {
      semantic_type_t te = (semantic_type_t)vec_get(to_elems, i);
      if (semantic_type_get_size(te) != semantic_type_get_size(elem) ||
          semantic_type_get_alignment(te) != semantic_type_get_alignment(elem))
        return false;
    }
    return true;
  }

  return false;
}

bool semantic_type_can_explicit_cast(semantic_type_t from, semantic_type_t to) {
  if (semantic_type_can_implicit_convert(from, to)) return true;
  return _explicit_cast_numeric(from, to) ||
         _explicit_cast_pointer(from, to) ||
         _explicit_cast_container(from, to);
}

/* ===== qualifier query utilities ===== */

bool semantic_type_is_const(semantic_type_t type) {
  if (!type || !type->impl) return false;
  return type->impl->kind == TYPE_QUALIFIER && type->impl->qualifier.is_const;
}

bool semantic_type_is_volatile(semantic_type_t type) {
  if (!type || !type->impl) return false;
  return type->impl->kind == TYPE_QUALIFIER && type->impl->qualifier.is_volatile;
}

semantic_type_t semantic_type_strip_qualifier(semantic_type_t type) {
  if (!type || !type->impl) return type;
  if (type->impl->kind == TYPE_QUALIFIER) return type->impl->qualifier.base;
  return type;
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
                                           size_t length,
                                           size_t length_param_idx) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_ARRAY);
  t->impl->array.element = element;
  t->impl->array.length = length;
  t->impl->array.length_param_idx = length_param_idx;
  t->is_incomplete = (length_param_idx != (size_t)-1);
  return t;
}

semantic_type_t semantic_type_create_qualifier(allocator_t allocator,
                                               semantic_type_t base,
                                               bool is_const, bool is_volatile) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_QUALIFIER);
  t->impl->qualifier.base = base;
  t->impl->qualifier.is_const = is_const;
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

semantic_type_t semantic_type_create_generic_instance(allocator_t allocator,
                                                       semantic_type_t template_type,
                                                       vec_t type_args) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_GENERIC_INSTANCE);
  t->impl->generic_instance.generic_template = template_type;
  t->impl->generic_instance.type_args = type_args;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_generic_param(allocator_t allocator,
                                                    const char *name,
                                                    size_t index,
                                                    semantic_type_t value_type,
                                                    bool is_value) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_GENERIC_PARAM);
  t->impl->generic_param.name = name;
  t->impl->generic_param.index = index;
  t->impl->generic_param.value_type = value_type;
  t->impl->generic_param.is_value = is_value;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_generic_pack(allocator_t allocator,
                                                   const char *name,
                                                   size_t index) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_GENERIC_PACK);
  t->impl->generic_pack.name = name;
  t->impl->generic_pack.index = index;
  vec_init_t vi = {.auto_dispose = false};
  t->impl->generic_pack.expanded_types =
      (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_pack_index(allocator_t allocator,
                                                const char *pack_name,
                                                size_t pack_param_idx,
                                                size_t index_param_idx) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_PACK_INDEX);
  t->impl->pack_index.pack_name = pack_name;
  t->impl->pack_index.pack_param_idx = pack_param_idx;
  t->impl->pack_index.index_param_idx = index_param_idx;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_generic_value(allocator_t allocator,
                                                    struct comptime_value *value) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_GENERIC_VALUE);
  t->impl->generic_value.value = value;
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_tuple(allocator_t allocator,
                                           vec_t element_types) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_TUPLE);
  t->impl->tuple.element_types = element_types;
  /* Pre-compute field symbols _0, _1, ... */
  vec_init_t vi = {.auto_dispose = true};
  t->impl->tuple.fields = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  size_t ecount = element_types ? vec_get_size(element_types) : 0;
  for (size_t i = 0; i < ecount; i++) {
    semantic_type_t et = (semantic_type_t)vec_get(element_types, i);
    char fname[16];
    snprintf(fname, sizeof(fname), "_%zu", i);
    struct symbol *fsym = symbol_create(allocator, fname, SYMBOL_FIELD, (location_t){0});
    fsym->field.type = et;
    fsym->field.index = i;
    fsym->field.is_pub = true;
    vec_push(t->impl->tuple.fields, fsym);
  }
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_opaque(allocator_t allocator) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_OPAQUE);
  t->is_incomplete = false;
  return t;
}

semantic_type_t semantic_type_create_wildcard(allocator_t allocator) {
  semantic_type_t t = (semantic_type_t)allocator_create(
      allocator, &g_semantic_type_type, NULL);
  t->impl = _create_impl(allocator, TYPE_WILDCARD);
  t->is_incomplete = false;
  return t;
}

