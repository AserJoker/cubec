#include "engine/char_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include <string.h>

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

uint64_t char_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  if (!data) return type_hash;
  uint8_t val;
  memcpy(&val, data, sizeof(uint8_t));
  return stype_hash_mix_u64(type_hash, (uint64_t)val);
}
