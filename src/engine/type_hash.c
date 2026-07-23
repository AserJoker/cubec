#include "engine/type_hash.h"
#include "engine/symbol.h"
#include "engine/comptime_value.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <string.h>

/* FNV-1a offset basis and prime for 64-bit */
#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

static size_t _fnv1a(const void *data, size_t len) {
  const unsigned char *p = (const unsigned char *)data;
  size_t hash = FNV_OFFSET;
  for (size_t i = 0; i < len; i++) {
    hash ^= (size_t)p[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

static size_t _hash_combine(size_t seed, size_t value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}

static size_t _hash_type(semantic_type_t type);

static size_t _hash_type_vec(vec_t vec) {
  size_t hash = FNV_OFFSET;
  size_t size = vec_get_size(vec);
  for (size_t i = 0; i < size; i++) {
    semantic_type_t t = (semantic_type_t)vec_get(vec, i);
    hash = _hash_combine(hash, _hash_type(t));
  }
  return hash;
}

static size_t _hash_type(semantic_type_t type) {
  if (!type || !type->impl) return FNV_OFFSET;

  type_impl_t impl = type->impl;
  size_t hash = _fnv1a(&impl->kind, sizeof(enum type_kind));

  switch (impl->kind) {
  case TYPE_VOID: case TYPE_BOOL:
  case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
  case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
  case TYPE_F16: case TYPE_F32: case TYPE_F64:
  case TYPE_CHAR: case TYPE_STRING: case TYPE_STR:
  case TYPE_NIL: case TYPE_ERROR:
    break;

  case TYPE_QUALIFIER:
    hash = _hash_combine(hash, _hash_type(impl->qualifier.base));
    hash = _hash_combine(hash, (size_t)impl->qualifier.is_const);
    hash = _hash_combine(hash, (size_t)impl->qualifier.is_volatile);
    break;

  case TYPE_POINTER:
    hash = _hash_combine(hash, _hash_type(impl->pointer.pointee));
    break;

  case TYPE_SLICE:
    hash = _hash_combine(hash, _hash_type(impl->slice.element));
    break;

  case TYPE_ARRAY:
    hash = _hash_combine(hash, _hash_type(impl->array.element));
    hash = _hash_combine(hash, impl->array.length);
    if (impl->array.length_param_name)
      hash = _hash_combine(hash, _fnv1a(impl->array.length_param_name,
                                         strlen(impl->array.length_param_name)));
    break;

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    /* Fields vec contains symbol*, not semantic_type_t.
       Hash based on field types. */
    vec_t fields = impl->struct_type.fields;
    size_t fcount = fields ? vec_get_size(fields) : 0;
    hash = _hash_combine(hash, fcount);
    for (size_t i = 0; i < fcount; i++) {
      struct symbol *field = (struct symbol *)vec_get(fields, i);
      if (field && field->field.type)
        hash = _hash_combine(hash, _hash_type(field->field.type));
    }
    break;
  }

  case TYPE_ENUM:
    hash = _hash_combine(hash, _hash_type(impl->enum_type.backing_type));
    break;

  case TYPE_INTERFACE: {
    /* Methods vec contains symbol*, not semantic_type_t.
       Hash based on method types. */
    vec_t methods = impl->interface_type.methods;
    size_t mcount = methods ? vec_get_size(methods) : 0;
    hash = _hash_combine(hash, mcount);
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *method = (struct symbol *)vec_get(methods, i);
      if (method && method->function.type)
        hash = _hash_combine(hash, _hash_type(method->function.type));
    }
    break;
  }

  case TYPE_FUNCTION:
    hash = _hash_combine(hash, _hash_type(impl->function.return_type));
    hash = _hash_combine(hash, _hash_type_vec(impl->function.params));
    hash = _hash_combine(hash, (size_t)impl->function.is_variadic);
    break;

  case TYPE_TYPE:
    hash = _hash_combine(hash, _hash_type(impl->type_of.inner));
    break;

  case TYPE_GENERIC_INSTANCE: {
    hash = _hash_combine(hash, _hash_type(impl->generic_instance.generic_template));
    /* Hash type_bindings: iterate deterministically by key name */
    strmap_t tb = impl->generic_instance.type_bindings;
    if (tb) {
      size_t bcount = strmap_get_size(tb);
      hash = _hash_combine(hash, bcount);
      strmap_iter_t it = strmap_iter_first(tb);
      const char *key;
      while ((key = strmap_iter_next(&it)) != NULL) {
        hash = _hash_combine(hash, _fnv1a(key, strlen(key)));
        semantic_type_t val = (semantic_type_t)strmap_find(tb, key);
        if (val) hash = _hash_combine(hash, _hash_type(val));
      }
    }
    break;
  }

  case TYPE_GENERIC_PARAM:
    hash = _hash_combine(hash, (size_t)impl->generic_param.is_value);
    if (impl->generic_param.name)
      hash = _hash_combine(hash, _fnv1a(impl->generic_param.name,
                                         strlen(impl->generic_param.name)));
    if (impl->generic_param.value_type)
      hash = _hash_combine(hash, _hash_type(impl->generic_param.value_type));
    break;

  case TYPE_GENERIC_PACK:
    if (impl->generic_pack.name)
      hash = _hash_combine(hash, _fnv1a(impl->generic_pack.name,
                                         strlen(impl->generic_pack.name)));
    hash = _hash_combine(hash, _hash_type_vec(impl->generic_pack.expanded_types));
    break;

  case TYPE_TUPLE:
    hash = _hash_combine(hash, _hash_type_vec(impl->tuple.element_types));
    break;

  case TYPE_OPAQUE:
    break;

  case TYPE_PACK_INDEX:
    if (impl->pack_index.pack_name)
      hash = _hash_combine(hash, _fnv1a(impl->pack_index.pack_name,
                                         strlen(impl->pack_index.pack_name)));
    if (impl->pack_index.index_param_name)
      hash = _hash_combine(hash, _fnv1a(impl->pack_index.index_param_name,
                                         strlen(impl->pack_index.index_param_name)));
    break;

  case TYPE_GENERIC_VALUE: {
    comptime_value_t cv = impl->generic_value.value;
    if (cv) {
      hash = _hash_combine(hash, (size_t)cv->kind);
      switch (cv->kind) {
      case COMPTIME_VALUE_INT:
        hash = _hash_combine(hash, cv->int_val.u);
        hash = _hash_combine(hash, (size_t)cv->int_val.is_signed);
        hash = _hash_combine(hash, (size_t)cv->int_val.width);
        break;
      case COMPTIME_VALUE_BOOL:
        hash = _hash_combine(hash, (size_t)cv->bool_val);
        break;
      case COMPTIME_VALUE_STRING:
        hash = _hash_combine(hash, _fnv1a(string_get(cv->string_val),
                                           string_get_length(cv->string_val)));
        break;
      case COMPTIME_VALUE_CHAR:
        hash = _hash_combine(hash, (size_t)cv->char_val);
        break;
      case COMPTIME_VALUE_FLOAT:
        hash = _hash_combine(hash, (size_t)cv->float_val.value);
        break;
      default:
        hash = _hash_combine(hash, (size_t)cv->kind);
        break;
      }
    }
    break;
  }

  case TYPE_WILDCARD:
    hash = _hash_combine(hash, (size_t)type->impl->wildcard.is_tuple);
    break;
  }

  return hash;
}

size_t type_hash_compute(semantic_type_t type) {
  return _hash_type(type);
}

void type_hash_ensure(semantic_type_t type) {
  if (!type || !type->impl) return;
  if (type->impl->hash == 0) {
    type->impl->hash = _hash_type(type);
  }
}
