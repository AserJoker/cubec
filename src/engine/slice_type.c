#include "engine/slice_type.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t slice_type_get_or_create(context_t ctx, stype_t element_type) {
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  uintptr_t elem_hash = (uintptr_t)element_type->instance.hash;
  vec_push(component_hashes, (void *)elem_hash);

  uint64_t hash = stype_compute_composite_hash(TYPE_SLICE, component_hashes);

  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  const char *elem_name =
      element_type->instance.name ? element_type->instance.name : "?";
  size_t name_len = 2 + strlen(elem_name);
  char *name = malloc(name_len + 1);
  snprintf(name, name_len + 1, "[]%s", elem_name);

  stype_t type = stype_create(ctx->allocator, TYPE_SLICE, NULL);
  type->instance.name = name;
  type->instance.hash = hash;
  type->instance.size = 24; /* ptr(8) + start(8) + len(8) */
  type->instance.align = 8;
  type->params = NULL;
  type->implements = NULL;

  rbtree_insert(ctx->types, hash, (void *)type);
  allocator_free(ctx->allocator, &component_hashes);
  return type;
}

bool type_kind_is_slice(enum type_kind_t kind) {
  return kind == TYPE_SLICE;
}

uint64_t slice_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  if (!data) return type_hash;
  /* slice layout: void* ptr; uint64_t start; uint64_t length; */
  const uint8_t *bytes = (const uint8_t *)data;
  uint64_t ptr_val, start_val, len_val;
  memcpy(&ptr_val, bytes, sizeof(uint64_t));
  memcpy(&start_val, bytes + sizeof(uint64_t), sizeof(uint64_t));
  memcpy(&len_val, bytes + 2 * sizeof(uint64_t), sizeof(uint64_t));
  uint64_t h = stype_hash_mix_u64(type_hash, ptr_val);
  h = stype_hash_mix_u64(h, start_val);
  h = stype_hash_mix_u64(h, len_val);
  return h;
}
