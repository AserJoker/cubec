#include "engine/slice_type.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t slice_type_get_or_create(context_t ctx, stype_t element_type) {
  /* Build component_type_hashes vector: just the element type hash */
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  uintptr_t elem_hash = (uintptr_t)element_type->instance.hash;
  vec_push(component_hashes, (void *)elem_hash);

  uint64_t hash = stype_compute_composite_hash(TYPE_SLICE, component_hashes);

  /* Check if already exists */
  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  /* Build name: "[]T" */
  const char *elem_name =
      element_type->instance.name ? element_type->instance.name : "?";
  size_t name_len = 2 + strlen(elem_name);
  char *name = malloc(name_len + 1);
  snprintf(name, name_len + 1, "[]%s", elem_name);

  /* Create the type: slice = pointer + length */
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

/* ---- Slice comptime value operations ---- */

void slice_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_SLICE) return;
  /* ptr is borrowing — do not dispose */
  allocator_free(val->allocator, &val);
}

comptime_value_t slice_type_clone_value(allocator_t allocator,
                                        comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_SLICE) return NULL;
  comptime_slice_t src = (comptime_slice_t)val;
  comptime_slice_t dst = allocator_alloc(allocator, sizeof(struct _comptime_slice_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->ptr = src->ptr;    /* borrowing */
  dst->start = src->start;
  dst->length = src->length;
  return (comptime_value_t)dst;
}

uint64_t slice_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_SLICE) return 0;
  comptime_slice_t v = (comptime_slice_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_SLICE);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, (uint64_t)(uintptr_t)v->ptr);
  h = stype_hash_mix_u64(h, v->start);
  h = stype_hash_mix_u64(h, v->length);
  return h;
}
