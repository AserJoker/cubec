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
