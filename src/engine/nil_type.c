#include "engine/nil_type.h"
#include "core/vec.h"
#include "core/type.h"

void nil_type_register(context_t ctx) {
  /* nil is an internal type — untyped pointer (void*), not visible to cubec code.
     Register in ctx->types for lifetime management, but do NOT insert into
     global_scope->names. */
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_NIL, "nil", 8, 8);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  ctx->t_nil = type;
}

stype_t nil_type_get(context_t ctx) {
  return ctx->t_nil;
}

comptime_value_t nil_type_create_value(context_t ctx) {
  comptime_nil_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_nil_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_NIL;
  v->header.type = ctx->t_nil;
  return (comptime_value_t)v;
}

void nil_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_NIL)
    return;
  allocator_free(val->allocator, &val);
}

comptime_value_t nil_type_clone_value(allocator_t allocator,
                                      comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_NIL)
    return NULL;
  comptime_nil_t src = (comptime_nil_t)val;
  comptime_nil_t dst = allocator_alloc(allocator, sizeof(struct _comptime_nil_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  return (comptime_value_t)dst;
}

uint64_t nil_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_NIL)
    return 0;
  uint64_t h = stype_compute_primitive_hash(TYPE_NIL);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  return h;
}
