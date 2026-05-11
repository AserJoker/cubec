#include "engine/array.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
struct _array_meta_t {
  size_t length;
  type_t type;
};
typedef struct _array_meta_t *array_meta_t;
static void array_meta_dispose(array_meta_t self, allocator_t allocator) {}
static array_meta_t create_array_meta(allocator_t allocator, type_t type,
                                      size_t len) {
  array_meta_t self = allocator_alloc(allocator, sizeof(struct _array_meta_t),
                                      (dispose_fn_t)array_meta_dispose);
  self->length = len;
  self->type = type;
  return self;
}

static value_t _array_set(value_t self, context_t ctx, value_t key,
                          value_t value) {
  type_t key_type = value_get_type(key);
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  uint64_t idx = 0;
  if (!value_is_mut(self)) {
    return create_error(ctx, "value is not mutable");
  }
  if (type_get_kind(key_type) == TYPE_KIND_INTEGER) {
    int64_t i = integer_get_value(key);
    if (i < 0) {
      return create_error(
          ctx, "index " PRIdPTR " is before the beginning of the array", i);
    } else {
      idx = i;
    }
  } else if (type_get_kind(key_type) == TYPE_KIND_UNSIGNED) {
    idx = unsigned_get_value(key);
  } else {
    return create_error(ctx, "array subscript is not an integer");
  }
  if (idx >= meta->length) {
    return create_error(
        ctx, "array index %" PRIuPTR " is past the end of the array", idx);
  }
  value = value_safe_convert(value, ctx, meta->type);
  if (value_is_error(value)) {
    return value;
  }
  if (!value_is_comptime(self)) {
    return context_create_value(ctx, meta->type, NULL, false, false, NULL);
  } else {
    void *data = value_get_data(self);
    void *value_data = value_get_data(value);
    memcpy((uint8_t *)data + type_get_size(meta->type) * idx, value_data,
           type_get_size(meta->type));
    return context_create_weak_value(
        ctx, meta->type, (uint8_t *)data + type_get_size(meta->type) * idx,
        false, NULL);
  }
}
static value_t _array_get(value_t self, context_t ctx, value_t key) {
  type_t key_type = value_get_type(key);
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  uint64_t idx = 0;
  if (type_get_kind(key_type) == TYPE_KIND_INTEGER) {
    int64_t i = integer_get_value(key);
    if (i < 0) {
      return create_error(
          ctx, "index " PRIdPTR " is before the beginning of the array", i);
    } else {
      idx = i;
    }
  } else if (type_get_kind(key_type) == TYPE_KIND_UNSIGNED) {
    idx = unsigned_get_value(key);
  } else {
    return create_error(ctx, "array subscript is not an integer");
  }
  if (idx >= meta->length) {
    return create_error(
        ctx, "array index %" PRIuPTR " is past the end of the array", idx);
  }
  bool mut = value_is_mut(self);
  if (!value_is_comptime(self)) {
    return context_create_value(ctx, meta->type, NULL, mut, false, NULL);
  } else {
    void *data = value_get_data(self);
    return context_create_weak_value(
        ctx, meta->type, (uint8_t *)data + type_get_size(meta->type) * idx, mut,
        NULL);
  }
}

static value_t _array_get_length(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  array_meta_t meta = type_get_meta(type);
  return create_comptime_u64(ctx, meta->length, false, NULL);
}

static value_t _array_safe_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_PARRAY) {
    type_t array_type = value_get_type(self);
    array_meta_t meta = type_get_meta(array_type);
    type_t ptr_type = ptr_type_get_type(type);
    if (type_is_equal(meta->type, ptr_type)) {
      if (value_is_comptime(self)) {
        void *data = value_get_data(self);
        return context_create_value(ctx, type, &data, false, true, NULL);
      } else {
        return context_create_value(ctx, type, NULL, false, false, NULL);
      }
    }
  }
  return create_error(ctx, "cannot convert %s to %s",
                      type_get_name(value_get_type(self)), type_get_name(type));
}

static bool _array_type_is_equal(type_t self, type_t another) {
  if (type_get_kind(another) == TYPE_KIND_ARRAY) {
    array_meta_t self_meta = type_get_meta(self);
    array_meta_t another_meta = type_get_meta(self);
    if (self_meta->length != another_meta->length) {
      return false;
    }
    return type_is_equal(self_meta->type, another_meta->type);
  }
  return false;
}
static value_t _array_slice(value_t self, context_t ctx, value_t start,
                            value_t end) {
  type_t type = value_get_type(self);
  type_t base_type = array_type_get_type(type);
  size_t array_len = array_type_get_length(type);
  type_t start_type = value_get_type(start);
  type_t end_type = value_get_type(end);
  type_t slice_type = create_slice_type(ctx, base_type, value_is_mut(self));
  if (value_is_comptime(self)) {
    size_t s = 0;
    size_t e = array_len;
    if (!value_is_comptime(start)) {
      return create_error(ctx, "slice start is not comptime");
    }
    if (!value_is_comptime(end)) {
      return create_error(ctx, "slice end is not comptime");
    }
    if (type_get_kind(start_type) != TYPE_KIND_VOID) {
      if (type_get_kind(start_type) == TYPE_KIND_INTEGER) {
        int64_t val = integer_get_value(start);
        if (val < 0) {
          return create_error(ctx, "slice start < 0");
        }
        s = val;
      } else if (type_get_kind(start_type) == TYPE_KIND_UNSIGNED) {
        s = unsigned_get_value(start);
      } else {
        return create_error(ctx, "slice start is not a integer");
      }
    }
    if (type_get_kind(end_type) != TYPE_KIND_VOID) {
      if (type_get_kind(end_type) == TYPE_KIND_INTEGER) {
        int64_t val = integer_get_value(end);
        if (val < 0) {
          return create_error(ctx, "slice end < 0");
        }
        e = val;
      } else if (type_get_kind(end_type) == TYPE_KIND_UNSIGNED) {
        e = unsigned_get_value(end);
      } else {
        return create_error(ctx, "slice end is not a integer");
      }
    }
    if (s >= array_len) {
      return create_error(ctx, "slice start %" PRIuPTR " >= %" PRIuPTR "", s,
                          array_len);
    }
    if (e > array_len) {
      return create_error(ctx, "slice end %" PRIuPTR " > %" PRIuPTR "", s,
                          array_len);
    }
    size_t len = 0;
    if (s < e) {
      len = e - s;
    }
    return create_comptime_slice(ctx, slice_type, value_get_data(self), s, len,
                                 value_is_mut(self));
  } else {
    return context_create_value(ctx, slice_type, NULL, false, false, NULL);
  }
}

type_t create_array_type(context_t ctx, type_t type, size_t length) {
  const char *base_id = type_get_id(type);
  size_t len = snprintf(NULL, 0, "A%sL%" PRIuPTR, base_id, length);
  char id[len];
  sprintf(id, "A%sL%" PRIuPTR, base_id, length);
  type_t self = context_load_type(ctx, id);
  if (!self) {
    const char *base_name = type_get_name(type);
    size_t len = snprintf(NULL, 0, "[%" PRIuPTR "]%s", length, base_name);
    char name[len];
    sprintf(name, "[%" PRIuPTR "]%s", length, base_name);
    allocator_t allocator = context_get_allocator(ctx);
    type_operator_t opt = {
        .type_eq = _array_type_is_equal,
        .addr_of = value_default_address_of,
        .get = _array_get,
        .set = _array_set,
        .get_length = _array_get_length,
        .safe_convert = _array_safe_convert,
        .slice = _array_slice,
    };
    array_meta_t meta = create_array_meta(allocator, type, length);
    self = create_type(allocator, TYPE_KIND_ARRAY, length * type_get_size(type),
                       type_get_align(type), name, id, &opt, meta);
    context_store_type(ctx, self);
    module_t module = context_get_module(ctx);
    module_add_type(module, self);
  }
  return self;
}
size_t array_type_get_length(type_t self) {
  array_meta_t meta = type_get_meta(self);
  return meta->length;
}
type_t array_type_get_type(type_t self) {
  array_meta_t meta = type_get_meta(self);
  return meta->type;
}