#include "engine/bool_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/type.h"

void bool_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_BOOL, "bool", 1, 1);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "bool", name);
  ctx->t_bool = type;
}

stype_t bool_type_get(context_t ctx) {
  return ctx->t_bool;
}

comptime_value_t bool_type_create_value(context_t ctx, bool val) {
  comptime_bool_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_bool_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_BOOL;
  v->header.type = ctx->t_bool;
  v->value = val;
  return (comptime_value_t)v;
}

bool bool_type_get_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_BOOL)
    return false;
  return ((comptime_bool_t)val)->value;
}

void bool_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_BOOL)
    return;
  allocator_free(val->allocator, &val);
}

comptime_value_t bool_type_clone_value(allocator_t allocator,
                                       comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_BOOL)
    return NULL;
  comptime_bool_t src = (comptime_bool_t)val;
  comptime_bool_t dst = allocator_alloc(allocator, sizeof(struct _comptime_bool_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->value = src->value;
  return (comptime_value_t)dst;
}

uint64_t bool_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_BOOL)
    return 0;
  comptime_bool_t v = (comptime_bool_t)val;
  uint64_t h = stype_compute_primitive_hash((enum type_kind_t)v->header.kind);
  if (v->header.type)
    h = stype_hash_mix_u64(h, v->header.type->instance.hash);
  h = stype_hash_mix_u64(h, (uint64_t)v->value);
  return h;
}
