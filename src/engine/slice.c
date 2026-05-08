#include "engine/slice.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef struct _slice_meta_t {
  type_t type;
} *slice_meta_t;

typedef struct _slice_data_t {
  size_t offset;
  size_t length;
  void *pdata;
} *slice_data_t;
static bool slice_type_is_eq(type_t self, type_t another) {
  if (type_get_kind(another) == TYPE_KIND_SLICE) {
    slice_meta_t src_meta = type_get_meta(self);
    slice_meta_t dst_meta = type_get_meta(another);
    return type_is_equal(src_meta->type, dst_meta->type);
  }
  return false;
}
static value_t slice_get_length(value_t self, context_t ctx) {
  if (value_is_comptime(self)) {
    slice_data_t data = value_get_data(self);
    return create_comptime_u64(ctx, data->length, false, NULL);
  }
  return create_u64(ctx, false, NULL);
}
static value_t slice_get(value_t self, context_t ctx, value_t key) {
  type_t key_type = value_get_type(key);
  type_t type = value_get_type(self);
  slice_meta_t meta = type_get_meta(type);
  uint64_t idx = 0;
  if (type_get_kind(key_type) == TYPE_KIND_INTEGER) {
    int64_t i = integer_get_value(key);
    if (i < 0) {
      return create_error(
          ctx, "index " PRIdPTR " is before the beginning of the slice", i);
    } else {
      idx = i;
    }
  } else if (type_get_kind(key_type) == TYPE_KIND_UNSIGNED) {
    idx = unsigned_get_value(key);
  } else {
    return create_error(ctx, "slice subscript is not an integer");
  }
  bool mut = value_is_mut(self);
  if (!value_is_comptime(self)) {
    return context_create_value(ctx, meta->type, NULL, mut, false, NULL);
  } else {
    slice_data_t data = value_get_data(self);
    if (idx >= data->length) {
      return create_error(
          ctx, "slice index %" PRIuPTR " is past the end of the slice", idx);
    }
    size_t offset = type_get_size(meta->type) * (idx + data->offset);
    uint8_t *start = (uint8_t *)(data->pdata);
    return context_create_weak_value(ctx, meta->type, start + offset, mut,
                                     NULL);
  }
}
static value_t slice_set(value_t self, context_t ctx, value_t key,
                         value_t value) {
  type_t key_type = value_get_type(key);
  type_t type = value_get_type(self);
  slice_meta_t meta = type_get_meta(type);
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
  value = value_safe_convert(value, ctx, meta->type);
  if (value_is_error(value)) {
    return value;
  }
  if (!value_is_comptime(self)) {
    return context_create_value(ctx, meta->type, NULL, false, false, NULL);
  } else {
    slice_data_t data = value_get_data(self);
    if (idx >= data->length) {
      return create_error(
          ctx, "array index %" PRIuPTR " is past the end of the array", idx);
    }
    void *dst = data->pdata;
    void *value_data = value_get_data(value);
    size_t offset = type_get_size(meta->type) * (idx + data->offset);
    memcpy((uint8_t *)dst + offset, value_data, type_get_size(meta->type));
    return context_create_weak_value(ctx, meta->type, (uint8_t *)dst + offset,
                                     false, NULL);
  }
}
static value_t slice_safe_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_PARRAY) {
    type_t array_type = value_get_type(self);
    slice_meta_t meta = type_get_meta(array_type);
    type_t ptr_type = ptr_type_get_type(type);
    if (type_is_equal(meta->type, ptr_type)) {
      if (value_is_comptime(self)) {
        slice_data_t data = value_get_data(self);
        void *src =
            (uint8_t *)data->pdata + data->offset * type_get_size(meta->type);
        return context_create_value(ctx, type, &src, false, true, NULL);
      } else {
        return context_create_value(ctx, type, NULL, false, false, NULL);
      }
    }
  }
  return create_error(ctx, "cannot convert %s to %s",
                      type_get_name(value_get_type(self)), type_get_name(type));
}

static value_t slice_slice(value_t self, context_t ctx, value_t start,
                           value_t end) {
  type_t type = value_get_type(self);
  type_t base_type = slice_type_get_type(type);
  size_t array_len = slice_get_len(self);
  type_t start_type = value_get_type(start);
  type_t end_type = value_get_type(end);
  type_t slice_type = create_slice_type(ctx, base_type);
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

static value_t slice_assigment(value_t self, context_t ctx, value_t value) {
  if (value_is_mut(self) && !value_is_mut(value)) {
    return create_error(ctx, "cannot assigment const value to mutable value");
  }
  return value_default_assigment(self, ctx, value);
}

type_t create_slice_type(context_t ctx, type_t base_type) {
  allocator_t allocator = context_get_allocator(ctx);
  const char *base_id = type_get_id(base_type);
  size_t len = snprintf(NULL, 0, "[]%s", base_id);
  char id[len];
  sprintf(id, "[]%s", base_id);
  type_t self = context_load_type(ctx, id);
  if (!self) {
    slice_meta_t meta =
        allocator_alloc(allocator, sizeof(struct _slice_meta_t), NULL);
    meta->type = base_type;
    type_operator_t opt = {
        .type_eq = slice_type_is_eq,
        .addr_of = value_default_address_of,
        .get = slice_get,
        .set = slice_set,
        .get_length = slice_get_length,
        .safe_convert = slice_safe_convert,
        .slice = slice_slice,
        .assigment = slice_assigment,
    };
    self = create_type(allocator, TYPE_KIND_SLICE, sizeof(struct _slice_data_t),
                       sizeof(void *), id, id, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}
value_t create_comptime_slice(context_t ctx, type_t type, void *pdata,
                              size_t offset, size_t len, bool mutable) {
  struct _slice_data_t data = {
      .offset = offset,
      .length = len,
      .pdata = pdata,
  };
  return context_create_value(ctx, type, &data, mutable, true, NULL);
}
type_t slice_type_get_type(type_t self) {
  slice_meta_t meta = type_get_meta(self);
  return meta->type;
}
size_t slice_get_len(value_t self) {
  slice_data_t data = value_get_data(self);
  return data->length;
}
size_t slice_get_offset(value_t self) {
  slice_data_t data = value_get_data(self);
  return data->offset;
}
void *slice_get_data(value_t self) {
  slice_data_t data = value_get_data(self);
  return data->pdata;
}