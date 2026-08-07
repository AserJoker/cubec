#include "engine/char_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"

void char_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_CHAR, "char", 4, 4);
  vec_push(ctx->types, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "char", name);
  ctx->t_char = type;
}

stype_t char_type_get(context_t ctx) {
  return ctx->t_char;
}
