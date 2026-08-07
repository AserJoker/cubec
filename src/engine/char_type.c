#include "engine/char_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/type.h"

void char_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_CHAR, "char", 1, 1);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "char", name);
  ctx->t_char = type;
}

stype_t char_type_get(context_t ctx) {
  return ctx->t_char;
}

comptime_value_t char_type_create_value(context_t ctx, uint64_t val) {
  comptime_int_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_int_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_INT;
  v->header.type = ctx->t_char;
  v->value = val;
  return (comptime_value_t)v;
}

uint64_t char_type_get_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_INT || !val->type ||
      val->type->type_kind != TYPE_CHAR)
    return 0;
  return ((comptime_int_t)val)->value;
}

void char_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_INT || !val->type ||
      val->type->type_kind != TYPE_CHAR)
    return;
  allocator_free(val->allocator, &val);
}

comptime_value_t char_type_clone_value(allocator_t allocator,
                                       comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_INT || !val->type ||
      val->type->type_kind != TYPE_CHAR)
    return NULL;
  comptime_int_t src = (comptime_int_t)val;
  comptime_int_t dst = allocator_alloc(allocator, sizeof(struct _comptime_int_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->value = src->value;
  return (comptime_value_t)dst;
}

uint64_t char_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_INT || !val->type ||
      val->type->type_kind != TYPE_CHAR)
    return 0;
  comptime_int_t v = (comptime_int_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_CHAR);
  h = stype_hash_mix_u64(h, v->header.type->instance.hash);
  h = stype_hash_mix_u64(h, v->value);
  return h;
}
