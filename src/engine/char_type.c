#include "engine/char_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/type.h"

void char_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_CHAR, "char", 1, 1);
  vec_push(ctx->types, (void *)type);
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
