#include "engine/bool_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"

void bool_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_BOOL, "bool", 1, 1);
  vec_push(ctx->types, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "bool", name);
  ctx->t_bool = type;
}

stype_t bool_type_get(context_t ctx) {
  return ctx->t_bool;
}
