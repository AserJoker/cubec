#include "engine/comptime_value.h"
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 *  Dispose — dispatch by kind
 * -------------------------------------------------------------------------- */

void comptime_value_dispose(comptime_value_t val) {
  if (!val) return;
  allocator_t allocator = val->allocator;
  switch (val->kind) {
  case COMPTIME_VALUE_INT: {
    comptime_int_t v = (comptime_int_t)val;
    allocator_free(allocator, &v);
    break;
  }
  case COMPTIME_VALUE_FLOAT: {
    comptime_float_t v = (comptime_float_t)val;
    allocator_free(allocator, &v);
    break;
  }
  case COMPTIME_VALUE_BOOL: {
    comptime_bool_t v = (comptime_bool_t)val;
    allocator_free(allocator, &v);
    break;
  }
  case COMPTIME_VALUE_STRING: {
    comptime_string_t v = (comptime_string_t)val;
    if (v->value)
      allocator_free(allocator, &v->value);
    allocator_free(allocator, &v);
    break;
  }
  case COMPTIME_VALUE_NIL: {
    comptime_nil_t v = (comptime_nil_t)val;
    allocator_free(allocator, &v);
    break;
  }
  case COMPTIME_VALUE_COMPOSITE: {
    comptime_composite_t v = (comptime_composite_t)val;
    if (v->fields)
      allocator_free(allocator, &v->fields);
    allocator_free(allocator, &v);
    break;
  }
  }
}

/* --------------------------------------------------------------------------
 *  Clone — dispatch by kind
 * -------------------------------------------------------------------------- */

comptime_value_t comptime_value_clone(allocator_t allocator, comptime_value_t val) {
  if (!val) return NULL;
  switch (val->kind) {
  case COMPTIME_VALUE_INT: {
    comptime_int_t src = (comptime_int_t)val;
    comptime_int_t dst = allocator_alloc(allocator, sizeof(struct _comptime_int_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    dst->value = src->value;
    return (comptime_value_t)dst;
  }
  case COMPTIME_VALUE_FLOAT: {
    comptime_float_t src = (comptime_float_t)val;
    comptime_float_t dst = allocator_alloc(allocator, sizeof(struct _comptime_float_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    dst->value = src->value;
    return (comptime_value_t)dst;
  }
  case COMPTIME_VALUE_BOOL: {
    comptime_bool_t src = (comptime_bool_t)val;
    comptime_bool_t dst = allocator_alloc(allocator, sizeof(struct _comptime_bool_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    dst->value = src->value;
    return (comptime_value_t)dst;
  }
  case COMPTIME_VALUE_STRING: {
    comptime_string_t src = (comptime_string_t)val;
    comptime_string_t dst = allocator_alloc(allocator, sizeof(struct _comptime_string_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    dst->value = src->value ? (string_t)value_clone(allocator, (void *)src->value) : NULL;
    return (comptime_value_t)dst;
  }
  case COMPTIME_VALUE_NIL: {
    comptime_nil_t src = (comptime_nil_t)val;
    comptime_nil_t dst = allocator_alloc(allocator, sizeof(struct _comptime_nil_t));
    dst->header = src->header;
    dst->header.allocator = allocator;
    return (comptime_value_t)dst;
  }
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
