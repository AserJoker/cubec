#include "engine/str_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"

void str_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_STR, "str", 0, 0);
  vec_push(ctx->types, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "str", name);
  ctx->t_str = type;
}

stype_t str_type_get(context_t ctx) {
  return ctx->t_str;
}
