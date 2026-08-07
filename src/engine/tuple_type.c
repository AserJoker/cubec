#include "engine/tuple_type.h"
#include "engine/context.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t tuple_type_get_or_create(context_t ctx, vec_t element_types) {
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  size_t n = element_types ? vec_get_size(element_types) : 0;
  uint64_t total_size = 0;
  uint64_t max_align = 1;
  for (size_t i = 0; i < n; i++) {
    stype_t et = (stype_t)vec_get(element_types, i);
    uintptr_t eh = (uintptr_t)et->instance.hash;
    vec_push(component_hashes, (void *)eh);
    total_size += et->instance.size;
    if (et->instance.align > max_align)
      max_align = et->instance.align;
  }

  uint64_t hash = stype_compute_composite_hash(TYPE_TUPLE, component_hashes);

  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  size_t name_cap = 64 + n * 16;
  char *name = malloc(name_cap);
  size_t pos = 0;
  name[pos++] = '(';
  for (size_t i = 0; i < n; i++) {
    stype_t et = (stype_t)vec_get(element_types, i);
    const char *en = et->instance.name ? et->instance.name : "?";
    size_t en_len = strlen(en);
    if (pos + en_len + 2 >= name_cap) {
      name_cap = name_cap * 2 + en_len + 2;
      name = realloc(name, name_cap);
    }
    memcpy(name + pos, en, en_len);
    pos += en_len;
    if (i + 1 < n)
      name[pos++] = ',';
  }
  name[pos++] = ')';
  name[pos] = '\0';

  stype_t type = stype_create(ctx->allocator, TYPE_TUPLE, NULL);
  type->instance.name = name;
  type->instance.hash = hash;
  type->instance.size = total_size;
  type->instance.align = max_align;
  type->params = NULL;
  type->implements = NULL;

  rbtree_insert(ctx->types, hash, (void *)type);
  allocator_free(ctx->allocator, &component_hashes);
  return type;
}

bool type_kind_is_tuple(enum type_kind_t kind) {
  return kind == TYPE_TUPLE;
}

uint64_t tuple_type_hash_value(context_t ctx, stype_t type, uint64_t type_hash, const void *data) {
  (void)ctx;
  if (!data) return type_hash;
  /* TODO: once tuple stores element_types, recurse into each element.
   * For now, hash the raw bytes as a fallback. */
  uint64_t h = type_hash;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < type->instance.size; i++)
    h = stype_hash_mix_u64(h, (uint64_t)bytes[i]);
  return h;
}
