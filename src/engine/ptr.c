#include "engine/ptr.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/slice.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct _ptr_meta_t *ptr_meta_t;
struct _ptr_meta_t {
  type_t type;
  bool mut;
  bool vol;
};
static void ptr_meta_dispose(ptr_meta_t self, allocator_t allocator) {}
static ptr_meta_t create_ptr_meta(allocator_t allocator, type_t type, bool mut,
                                  bool vol) {
  ptr_meta_t self = allocator_alloc(allocator, sizeof(struct _ptr_meta_t),
                                    (dispose_fn_t)ptr_meta_dispose);
  self->mut = mut;
  self->vol = vol;
  self->type = type;
  return self;
}
static value_t ptr_assigment(value_t self, context_t ctx, value_t value) {
  type_t type = value_get_type(self);
  if (!ptr_type_is_mut(type)) {
    return create_error(ctx, "value is not mutable");
  }
  value = value_safe_convert(value, ctx, type);
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_mut(self) && !value_is_mut(value)) {
    return create_error(ctx, "cannot assigment const value to mutable value");
  }
  if (value_is_comptime(self)) {
    if (value_is_comptime(value)) {
      void **dst = (void **)value_get_data(self);
      void **src = (void **)value_get_data(value);
      *dst = *src;
      return self;
    } else {
      return create_error(ctx, "value is not comptime");
    }
  } else {
    return self;
  }
}
static value_t ptr_eq(value_t self, context_t ctx, value_t value) {
  type_t left_type = value_get_type(self);
  value = value_safe_convert(value, ctx, left_type);
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_comptime(self) && value_is_comptime(value)) {
    void *left = *(void **)value_get_data(self);
    void *right = *(void **)value_get_data(value);
    return create_comptime_bool(ctx, left == right, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t ptr_ne(value_t self, context_t ctx, value_t value) {
  type_t left_type = value_get_type(self);
  value = value_safe_convert(value, ctx, left_type);
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_comptime(self) && value_is_comptime(value)) {
    void *left = *(void **)value_get_data(self);
    void *right = *(void **)value_get_data(value);
    return create_comptime_bool(ctx, left != right, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t ptr_deref(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  type_t base_type = ptr_type_get_type(type);
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_weak_value(ctx, base_type, data, mut, NULL);
  } else {
    return context_create_value(ctx, base_type, NULL, mut, false, NULL);
  }
}
static value_t ptr_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_PTR &&
      type_get_kind(type) != TYPE_KIND_PARRAY &&
      type_get_kind(type) != TYPE_KIND_OPAQUE) {
    return create_error(ctx, "cannot convert %s to %s",
                        type_get_name(value_type), type_get_name(type));
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_value(ctx, type, &data, mut, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}
static value_t ptr_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_PTR) {
    return create_error(ctx, "cannot convert %s to %s",
                        type_get_name(value_type), type_get_name(type));
  }
  type_t src_type = ptr_type_get_type(value_type);
  type_t dst_type = ptr_type_get_type(type);
  if (type_get_kind(src_type) != type_get_kind(dst_type)) {
    return create_error(ctx, "cannot convert %s to %s",
                        type_get_name(value_type), type_get_name(type));
  }
  if (type_get_size(src_type) < type_get_size(dst_type)) {
    return create_error(ctx, "cannot convert %s to %s",
                        type_get_name(value_type), type_get_name(type));
  }
  if (type_get_align(src_type) != type_get_align(dst_type)) {
    return create_error(ctx, "cannot convert %s to %s",
                        type_get_name(value_type), type_get_name(type));
  }
  if (type_get_kind(src_type) == TYPE_KIND_STRUCT) {
    array_t src_fields = struct_type_get_fields(src_type);
    array_t dst_fields = struct_type_get_fields(src_type);
    if (array_get_size(src_fields) > array_get_size(dst_fields)) {
      return create_error(ctx, "cannot convert %s to %s",
                          type_get_name(value_type), type_get_name(type));
    }
    for (size_t idx = 0; idx < array_get_size(src_fields); idx++) {
      struct_field_t src_field = array_get(src_fields, idx);
      struct_field_t dst_field = array_get(dst_fields, idx);
      if (src_field->offset != dst_field->offset ||
          strcmp(src_field->name, dst_field->name) != 0 ||
          !type_is_equal(src_field->type, dst_field->type)) {
        return create_error(ctx, "cannot convert %s to %s",
                            type_get_name(value_type), type_get_name(type));
      }
    }
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_value(ctx, type, &data, mut, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}
static value_t ptr_get_field(value_t self, context_t ctx, const char *name) {
  value_t base = value_deref(self, ctx);
  return value_get_field(base, ctx, name);
}
static value_t ptr_set_field(value_t self, context_t ctx, const char *name,
                             value_t value) {
  value_t base = value_deref(self, ctx);
  return value_set_field(base, ctx, name, value);
}
static bool _ptr_type_is_equal(type_t self, type_t another) {
  if (type_get_kind(self) != type_get_kind(another)) {
    return false;
  }
  return type_is_equal(ptr_type_get_type(self), ptr_type_get_type(another));
}
type_t create_ptr_type(context_t ctx, type_t type, bool mut, bool vol) {
  const char *base_id = type_get_id(type);
  size_t len = snprintf(NULL, 0, "*%s%s%s", !mut ? "const " : "",
                        vol ? "volatile " : "", base_id);
  char id[len + 1];
  sprintf(id, "*%s%s%s", !mut ? "const " : "", vol ? "volatile " : "", base_id);
  type_t self = context_load_type(ctx, id);
  if (!self) {
    const char *base_name = type_get_name(type);
    size_t len = snprintf(NULL, 0, "*%s%s%s", !mut ? "const " : "",
                          vol ? "volatile " : "", base_name);
    char name[len + 1];
    sprintf(name, "*%s%s%s", !mut ? "const " : "", vol ? "volatile " : "",
            base_name);
    allocator_t allocator = context_get_allocator(ctx);
    ptr_meta_t meta = create_ptr_meta(allocator, type, mut, vol);
    type_operator_t opt = {
        .type_eq = _ptr_type_is_equal,
        .addr_of = value_default_address_of,
        .deref = ptr_deref,
        .eq = ptr_eq,
        .ne = ptr_ne,
        .convert = ptr_convert,
        .safe_convert = ptr_safe_convert,
        .assigment = ptr_assigment,
        .get_field = ptr_get_field,
        .set_field = ptr_set_field,
    };
    self = create_type(allocator, TYPE_KIND_PTR, sizeof(void *), sizeof(void *),
                       name, id, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}
static value_t parray_get(value_t self, context_t ctx, value_t key) {
  type_t key_type = value_get_type(key);
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
  type_t type = value_get_type(self);
  type_t base_type = ptr_type_get_type(type);
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_weak_value(
        ctx, base_type, (uint8_t *)data + idx * type_get_size(base_type), mut,
        NULL);
  } else {
    return context_create_value(ctx, base_type, NULL, mut, false, NULL);
  }
}
static value_t parray_set(value_t self, context_t ctx, value_t key,
                          value_t value) {
  type_t key_type = value_get_type(key);
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
  type_t type = value_get_type(self);
  type_t base_type = ptr_type_get_type(type);
  value = value_safe_convert(value, ctx, base_type);
  if (value_is_error(value)) {
    return value;
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    if (value_is_comptime(value)) {
      memcpy((uint8_t *)data + idx * type_get_size(base_type),
             value_get_data(value), type_get_size(base_type));
    } else {
      return create_error(ctx, "value is not comptime");
    }
  }
  return value;
}
static value_t parray_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_OPAQUE &&
      type_get_kind(type) != TYPE_KIND_PARRAY &&
      type_get_kind(type) != TYPE_KIND_PTR) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_value(ctx, type, &data, mut, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}
static value_t parray_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_OPAQUE &&
      type_get_kind(type) != TYPE_KIND_PARRAY) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  if (type_get_kind(type) == TYPE_KIND_PARRAY) {
    type_t src_type = ptr_type_get_type(type);
    type_t dst_type = ptr_type_get_type(value_type);
    if (strcmp(type_get_id(src_type), type_get_id(dst_type)) != 0) {
      return create_error(ctx, "cannot convert '%s' to '%s'",
                          type_get_name(value_type), type_get_name(type));
    }
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_value(ctx, type, &data, mut, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}
static value_t parray_slice(value_t self, context_t ctx, value_t start,
                            value_t end) {
  type_t type = value_get_type(self);
  type_t base_type = ptr_type_get_type(type);
  type_t start_type = value_get_type(start);
  type_t end_type = value_get_type(end);
  type_t slice_type = create_slice_type(ctx, base_type);
  if (value_is_comptime(self)) {
    size_t s = 0;
    size_t e = 0;
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
type_t create_parray_type(context_t ctx, type_t type, bool mut, bool vol) {
  const char *base_name = type_get_id(type);
  size_t len = snprintf(NULL, 0, "[*]%s%s%s", mut ? "const " : "",
                        vol ? "volatile " : "", base_name);
  char buf[len + 1];
  sprintf(buf, "[*]%s%s%s", !mut ? "const " : "", vol ? "volatile " : "",
          base_name);
  type_t self = context_load_type(ctx, buf);
  if (!self) {
    allocator_t allocator = context_get_allocator(ctx);
    ptr_meta_t meta = create_ptr_meta(allocator, type, mut, vol);
    type_operator_t opt = {
        .addr_of = value_default_address_of,
        .eq = ptr_eq,
        .ne = ptr_ne,
        .get = parray_get,
        .set = parray_set,
        .convert = parray_convert,
        .safe_convert = parray_safe_convert,
        .slice = parray_slice,
        .assigment = ptr_assigment,
    };
    self = create_type(allocator, TYPE_KIND_PARRAY, sizeof(void *),
                       sizeof(void *), buf, buf, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}
value_t create_ptr_value(context_t ctx, value_t src) {
  type_t type = value_get_type(src);
  type_t ptr_type = create_ptr_type(ctx, type, true, false);
  bool mut = value_is_mut(src);
  if (value_is_comptime(src)) {
    void *data = value_get_data(src);
    return context_create_value(ctx, ptr_type, &data, mut, true, NULL);
  } else {
    return context_create_value(ctx, ptr_type, NULL, mut, false, NULL);
  }
}
type_t ptr_type_get_type(type_t type) {
  ptr_meta_t meta = type_get_meta(type);
  return meta->type;
}
bool ptr_type_is_mut(type_t type) {
  ptr_meta_t meta = type_get_meta(type);
  return meta->mut;
}
bool ptr_type_is_vol(type_t type) {
  ptr_meta_t meta = type_get_meta(type);
  return meta->vol;
}