#include "engine/array_type.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t array_type_get_or_create(context_t ctx, stype_t element_type,
                                  uint64_t length) {
  /* Build component_type_hashes: element hash + length as hash component */
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  uintptr_t elem_hash = (uintptr_t)element_type->instance.hash;
  vec_push(component_hashes, (void *)elem_hash);
  uintptr_t len_hash = (uintptr_t)length;
  vec_push(component_hashes, (void *)len_hash);

  uint64_t hash = stype_compute_composite_hash(TYPE_ARRAY, component_hashes);

  /* Check if already exists */
  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  /* Build name: "[N]T" */
  const char *elem_name =
      element_type->instance.name ? element_type->instance.name : "?";
  size_t name_len = 32 + strlen(elem_name);
  char *name = malloc(name_len + 1);
  snprintf(name, name_len + 1, "[%llu]%s", (unsigned long long)length,
           elem_name);

  /* Create the type */
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
