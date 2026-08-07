#include "engine/tuple_type.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t tuple_type_get_or_create(context_t ctx, vec_t element_types) {
  /* Build component_type_hashes from element types */
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

  /* Check if already exists */
  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  /* Build name: "(A,B,C)" */
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

/* ---- Tuple comptime value operations ---- */

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

void tuple_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_TUPLE) return;
  comptime_tuple_t v = (comptime_tuple_t)val;
  _dispose_value_vec(v->elements, val->allocator);
  allocator_free(val->allocator, &val);
}

comptime_value_t tuple_type_clone_value(allocator_t allocator,
                                        comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_TUPLE) return NULL;
  comptime_tuple_t src = (comptime_tuple_t)val;
  comptime_tuple_t dst = allocator_alloc(allocator, sizeof(struct _comptime_tuple_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->elements = _clone_value_vec(allocator, src->elements);
  return (comptime_value_t)dst;
}

uint64_t tuple_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_TUPLE) return 0;
  comptime_tuple_t v = (comptime_tuple_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_TUPLE);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, _hash_value_vec(v->elements));
  return h;
}
