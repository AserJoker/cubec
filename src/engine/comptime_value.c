#include "engine/comptime_value.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/bool_type.h"
#include "engine/char_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/stype.h"
#include "core/vec.h"
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 *  Dispose — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

void comptime_value_dispose(comptime_value_t val) {
  if (!val) return;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    /* char and integer share COMPTIME_VALUE_INT — dispatch by type_kind */
    if (val->type && val->type->type_kind == TYPE_CHAR)
      char_type_dispose_value(val);
    else
      integer_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_FLOAT:
    float_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_BOOL:
    bool_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_STRING:
    str_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_NIL:
    nil_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_COMPOSITE: {
    comptime_composite_t v = (comptime_composite_t)val;
    if (v->fields) {
      size_t n = vec_get_size(v->fields);
      for (size_t i = 0; i < n; i++)
        comptime_value_dispose((comptime_value_t)vec_get(v->fields, i));
      allocator_free(val->allocator, &v->fields);
    }
    allocator_free(val->allocator, &val);
    break;
  }
  }
}

/* --------------------------------------------------------------------------
 *  Clone — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

comptime_value_t comptime_value_clone(allocator_t allocator, comptime_value_t val) {
  if (!val) return NULL;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    if (val->type && val->type->type_kind == TYPE_CHAR)
      return char_type_clone_value(allocator, val);
    return integer_type_clone_value(allocator, val);
  case COMPTIME_VALUE_FLOAT:
    return float_type_clone_value(allocator, val);
  case COMPTIME_VALUE_BOOL:
    return bool_type_clone_value(allocator, val);
  case COMPTIME_VALUE_STRING:
    return str_type_clone_value(allocator, val);
  case COMPTIME_VALUE_NIL:
    return nil_type_clone_value(allocator, val);
  case COMPTIME_VALUE_COMPOSITE: {
    comptime_composite_t src = (comptime_composite_t)val;
    comptime_composite_t dst = allocator_alloc(allocator, sizeof(struct _comptime_composite_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    if (src->fields) {
      vec_init_t vi = {.auto_dispose = true};
      dst->fields = allocator_create(allocator, &g_vec_type, &vi);
      size_t n = vec_get_size(src->fields);
      for (size_t i = 0; i < n; i++) {
        comptime_value_t f = comptime_value_clone(allocator, vec_get(src->fields, i));
        vec_push(dst->fields, f);
      }
    } else {
      dst->fields = NULL;
    }
    return (comptime_value_t)dst;
  }
  }
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Hash — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

uint64_t comptime_value_hash(comptime_value_t val) {
  if (!val) return 0;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    if (val->type && val->type->type_kind == TYPE_CHAR)
      return char_type_hash_value(val);
    return integer_type_hash_value(val);
  case COMPTIME_VALUE_FLOAT:
    return float_type_hash_value(val);
  case COMPTIME_VALUE_BOOL:
    return bool_type_hash_value(val);
  case COMPTIME_VALUE_STRING:
    return str_type_hash_value(val);
  case COMPTIME_VALUE_NIL:
    return nil_type_hash_value(val);
  case COMPTIME_VALUE_COMPOSITE: {
    comptime_composite_t v = (comptime_composite_t)val;
    uint64_t h = stype_compute_primitive_hash((enum type_kind_t)val->kind);
    if (val->type)
      h = stype_hash_mix_u64(h, val->type->instance.hash);
    size_t n = v->fields ? vec_get_size(v->fields) : 0;
    h = stype_hash_mix_u64(h, n);
    for (size_t i = 0; i < n; i++) {
      comptime_value_t f = (comptime_value_t)vec_get(v->fields, i);
      h = stype_hash_mix_u64(h, comptime_value_hash(f));
    }
    return h;
  }
  }
  return 0;
}
