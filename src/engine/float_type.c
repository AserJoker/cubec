#include "engine/float_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include <string.h>

static const struct {
  enum type_kind_t kind;
  const char *name;
  uint64_t size;
  uint64_t align;
} _float_types[] = {
    {TYPE_F16, "f16", 2, 2},
    {TYPE_F32, "f32", 4, 4},
    {TYPE_F64, "f64", 8, 8},
};
#define NUM_FLOAT_TYPES (sizeof(_float_types) / sizeof(_float_types[0]))

void float_types_register(context_t ctx) {
  allocator_t allocator = ctx->allocator;
  scope_t scope = ctx->global_scope;
  for (size_t i = 0; i < NUM_FLOAT_TYPES; i++) {
    stype_t type = stype_create_primitive(allocator, _float_types[i].kind,
                                          _float_types[i].name,
                                          _float_types[i].size,
                                          _float_types[i].align);
    rbtree_insert(ctx->types, type->instance.hash, (void *)type);
    name_t name = name_create(allocator, NAME_TYPE, (void *)type);
    strmap_insert(scope->names, _float_types[i].name, name);
    switch (_float_types[i].kind) {
    case TYPE_F16: ctx->t_f16 = type; break;
    case TYPE_F32: ctx->t_f32 = type; break;
    case TYPE_F64: ctx->t_f64 = type; break;
    default: break;
    }
  }
}

stype_t float_type_get(context_t ctx, enum type_kind_t kind) {
  switch (kind) {
  case TYPE_F16: return ctx->t_f16;
  case TYPE_F32: return ctx->t_f32;
  case TYPE_F64: return ctx->t_f64;
  default:       return NULL;
  }
}

uint64_t float_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  if (!data) return type_hash;
  /* Read the IEEE 754 bit representation according to its size */
  uint64_t bits = 0;
  memcpy(&bits, data, type->instance.size);
  return stype_hash_mix_u64(type_hash, bits);
}

bool type_kind_is_float(enum type_kind_t kind) {
  return kind == TYPE_F16 || kind == TYPE_F32 || kind == TYPE_F64;
}
