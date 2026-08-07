#include "engine/nil_type.h"
#include "core/vec.h"

void nil_type_register(context_t ctx) {
  /* nil is an internal type — untyped pointer (void*), not visible to cubec code.
     Register in ctx->types for lifetime management, but do NOT insert into
     global_scope->names. */
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_NIL, "nil", 8, 8);
  vec_push(ctx->types, (void *)type);
  ctx->t_nil = type;
}

stype_t nil_type_get(context_t ctx) {
  return ctx->t_nil;
}
