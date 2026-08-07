#include "engine/callable_type.h"
#include "core/allocator.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include "engine/void_type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stype_t callable_type_get_or_create(context_t ctx, vec_t param_types,
                                     stype_t return_type) {
  /* Build component_type_hashes: param type hashes + return type hash */
  vec_init_t vi = {.auto_dispose = false};
  vec_t component_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  size_t param_count = param_types ? vec_get_size(param_types) : 0;
  for (size_t i = 0; i < param_count; i++) {
    stype_t pt = (stype_t)vec_get(param_types, i);
    uintptr_t ph = (uintptr_t)pt->instance.hash;
    vec_push(component_hashes, (void *)ph);
  }

  /* Return type is always part of the hash (even void) */
  uintptr_t ret_hash = (uintptr_t)return_type->instance.hash;
  vec_push(component_hashes, (void *)ret_hash);

  uint64_t hash = stype_compute_composite_hash(TYPE_CALLABLE, component_hashes);

  /* Check if already exists */
  stype_t existing = (stype_t)rbtree_find(ctx->types, hash);
  if (existing) {
    allocator_free(ctx->allocator, &component_hashes);
    return existing;
  }

  /* Build name: "fn(A,B)R" or "fn(A,B)" if void return */
  size_t name_cap = 64 + param_count * 16;
  char *name = malloc(name_cap);
  size_t pos = 0;
  name[pos++] = 'f';
  name[pos++] = 'n';
  name[pos++] = '(';

  for (size_t i = 0; i < param_count; i++) {
    stype_t pt = (stype_t)vec_get(param_types, i);
    const char *pn = pt->instance.name ? pt->instance.name : "?";
    size_t pn_len = strlen(pn);
    /* Ensure capacity */
    if (pos + pn_len + 2 >= name_cap) {
      name_cap = name_cap * 2 + pn_len + 2;
      name = realloc(name, name_cap);
    }
    memcpy(name + pos, pn, pn_len);
    pos += pn_len;
    if (i + 1 < param_count) {
      name[pos++] = ',';
    }
  }
  name[pos++] = ')';

  bool is_void_return = (return_type->type_kind == TYPE_VOID);
  if (!is_void_return) {
    const char *rn =
        return_type->instance.name ? return_type->instance.name : "?";
    size_t rn_len = strlen(rn);
    if (pos + rn_len + 1 >= name_cap) {
      name_cap = name_cap * 2 + rn_len + 1;
      name = realloc(name, name_cap);
    }
    memcpy(name + pos, rn, rn_len);
    pos += rn_len;
  }
  name[pos] = '\0';

  /* Create the type */
  stype_t type = stype_create(ctx->allocator, TYPE_CALLABLE, NULL);
  type->instance.name = name;
  type->instance.hash = hash;
  type->instance.size = 8;  /* function pointer */
  type->instance.align = 8;
  type->params = NULL;
  type->implements = NULL;

  rbtree_insert(ctx->types, hash, (void *)type);
  allocator_free(ctx->allocator, &component_hashes);
  return type;
}

bool type_kind_is_callable(enum type_kind_t kind) {
  return kind == TYPE_CALLABLE;
}

/* ---- Callable comptime value operations ---- */

void callable_type_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CALLABLE) return;
  /* function_ref is borrowing */
  allocator_free(val->allocator, &val);
}

comptime_value_t callable_type_clone_value(allocator_t allocator,
                                           comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CALLABLE) return NULL;
  comptime_callable_t src = (comptime_callable_t)val;
  comptime_callable_t dst = allocator_alloc(allocator, sizeof(struct _comptime_callable_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->function_ref = src->function_ref;  /* borrowing */
  dst->instance_hash = src->instance_hash;
  return (comptime_value_t)dst;
}

uint64_t callable_type_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CALLABLE) return 0;
  comptime_callable_t v = (comptime_callable_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_CALLABLE);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, (uint64_t)(uintptr_t)v->function_ref);
  h = stype_hash_mix_u64(h, v->instance_hash);
  return h;
}
