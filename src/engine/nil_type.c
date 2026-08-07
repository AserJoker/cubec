#include "engine/nil_type.h"
#include <string.h>

void nil_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_NIL, "nil", 8, 8);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  ctx->t_nil = type;
}

stype_t nil_type_get(context_t ctx) {
  return ctx->t_nil;
}

uint64_t nil_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  if (!data) return type_hash;
  /* nil stores a pointer value (typed null) */
  uint64_t val;
  memcpy(&val, data, sizeof(uint64_t));
  return stype_hash_mix_u64(type_hash, val);
}
