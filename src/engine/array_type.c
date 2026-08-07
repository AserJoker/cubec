#include "engine/array_type.h"
#include "engine/context.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t array_type_get_or_create(context_t ctx, stype_t element_type,
                                  uint64_t length) {
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  uintptr_t elem_hash = (uintptr_t)element_type->instance.hash;
  vec_push(component_hashes, (void *)elem_hash);
  uintptr_t len_hash = (uintptr_t)length;
  vec_push(component_hashes, (void *)len_hash);

  uint64_t hash = stype_compute_composite_hash(TYPE_ARRAY, component_hashes);

  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  const char *elem_name =
      element_type->instance.name ? element_type->instance.name : "?";
  size_t name_len = 32 + strlen(elem_name);
  char *name = malloc(name_len + 1);
  snprintf(name, name_len + 1, "[%llu]%s", (unsigned long long)length,
           elem_name);

  stype_t type = stype_create(ctx->allocator, TYPE_ARRAY, NULL);
  type->instance.name = name;
  type->instance.hash = hash;
  type->instance.size = element_type->instance.size * length;
  type->instance.align = element_type->instance.align;
  type->params = NULL;
  type->implements = NULL;

  rbtree_insert(ctx->types, hash, (void *)type);
  allocator_free(ctx->allocator, &component_hashes);
  return type;
}

bool type_kind_is_array(enum type_kind_t kind) {
  return kind == TYPE_ARRAY;
}

uint64_t array_type_hash_value(context_t ctx, stype_t type, uint64_t type_hash, const void *data) {
  (void)ctx;
  if (!data) return type_hash;
  /* TODO: once array stores element_type, recurse into each element.
   * For now, hash the raw bytes as a fallback. */
  uint64_t h = type_hash;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < type->instance.size; i++)
    h = stype_hash_mix_u64(h, (uint64_t)bytes[i]);
  return h;
}
