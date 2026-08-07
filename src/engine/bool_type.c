#include "engine/bool_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include <string.h>

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

uint64_t bool_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  if (!data) return type_hash;
  bool val;
  memcpy(&val, data, sizeof(bool));
  return stype_hash_mix_u64(type_hash, (uint64_t)val);
}
