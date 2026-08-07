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

/* ---- Array comptime value operations ---- */

static void _dispose_value_vec(vec_t vec, allocator_t allocator) {
  if (!vec) return;
  size_t n = vec_get_size(vec);
  for (size_t i = 0; i < n; i++)
    comptime_value_dispose((comptime_value_t)vec_get(vec, i));
  allocator_free(allocator, &vec);
}

static vec_t _clone_value_vec(allocator_t allocator, vec_t src) {
  if (!src) return NULL;
  vec_init_t vi = {.auto_dispose = true};
  vec_t dst = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  size_t n = vec_get_size(src);
  for (size_t i = 0; i < n; i++) {
    comptime_value_t f = comptime_value_clone(allocator, vec_get(src, i));
    vec_push(dst, f);
  }
  return dst;
}

static uint64_t _hash_value_vec(vec_t vec) {
  if (!vec) return 0;
  uint64_t h = 0;
  size_t n = vec_get_size(vec);
  h = stype_hash_mix_u64(h, n);
  for (size_t i = 0; i < n; i++) {
    comptime_value_t f = (comptime_value_t)vec_get(vec, i);
    h = stype_hash_mix_u64(h, comptime_value_hash(f));
  }
  return h;
}

void array_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_ARRAY) return;
  comptime_array_t v = (comptime_array_t)val;
  _dispose_value_vec(v->elements, val->allocator);
  allocator_free(val->allocator, &val);
}

comptime_value_t array_type_clone_value(allocator_t allocator,
                                        comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_ARRAY) return NULL;
  comptime_array_t src = (comptime_array_t)val;
  comptime_array_t dst = allocator_alloc(allocator, sizeof(struct _comptime_array_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->elements = _clone_value_vec(allocator, src->elements);
  return (comptime_value_t)dst;
}

uint64_t array_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_ARRAY) return 0;
  comptime_array_t v = (comptime_array_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_ARRAY);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, _hash_value_vec(v->elements));
  return h;
}
