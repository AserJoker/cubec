#include "engine/str_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/string.h"
#include "core/rbtree.h"
#include <string.h>

void str_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_STR, "str", 0, 0);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "str", name);
  ctx->t_str = type;
}

stype_t str_type_get(context_t ctx) {
  return ctx->t_str;
}

uint64_t str_type_hash_value(context_t ctx_, stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  if (!data) return type_hash;
  context_t ctx = (context_t)ctx_;
  /* str buffer stores a string_id (uint64_t) — look up in context string table */
  uint64_t string_id;
  memcpy(&string_id, data, sizeof(uint64_t));
  if (string_id == 0)
    return type_hash;
  string_t s = (string_t)rbtree_find(ctx->strings, string_id);
  if (!s)
    return stype_hash_mix_u64(type_hash, string_id);
  const char *str = string_get(s);
  size_t len = strlen(str);
  uint64_t h = stype_hash_mix_u64(type_hash, len);
  for (size_t i = 0; i < len; i++)
    h = stype_hash_mix_u64(h, (uint64_t)(uint8_t)str[i]);
  return h;
}
