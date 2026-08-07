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

comptime_value_t float_type_create_value(context_t ctx, enum type_kind_t kind, double val) {
  stype_t type = float_type_get(ctx, kind);
  comptime_float_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_float_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_FLOAT;
  v->header.type = type;
  v->value = val;
  return (comptime_value_t)v;
}

double float_type_get_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FLOAT)
    return 0.0;
  return ((comptime_float_t)val)->value;
}

bool type_kind_is_float(enum type_kind_t kind) {
  return kind == TYPE_F16 || kind == TYPE_F32 || kind == TYPE_F64;
}

void float_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FLOAT)
    return;
  allocator_free(val->allocator, &val);
}

comptime_value_t float_type_clone_value(allocator_t allocator,
                                        comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FLOAT)
    return NULL;
  comptime_float_t src = (comptime_float_t)val;
  comptime_float_t dst = allocator_alloc(allocator, sizeof(struct _comptime_float_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->value = src->value;
  return (comptime_value_t)dst;
}

uint64_t float_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FLOAT)
    return 0;
  comptime_float_t v = (comptime_float_t)val;
  uint64_t h = stype_compute_primitive_hash((enum type_kind_t)v->header.kind);
  if (v->header.type)
    h = stype_hash_mix_u64(h, v->header.type->instance.hash);
  uint64_t bits;
  memcpy(&bits, &v->value, sizeof(bits));
  h = stype_hash_mix_u64(h, bits);
  return h;
}
