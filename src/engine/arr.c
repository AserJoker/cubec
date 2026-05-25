#include "engine/arr.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
struct _arr_meta_t {
  size_t length;
  type_t type;
};
typedef struct _arr_meta_t *arr_meta_t;

static arr_meta_t create_arr_meta(allocator_t allocator, type_t type,
                                  size_t length) {
  arr_meta_t self =
      allocator_alloc(allocator, sizeof(struct _arr_meta_t), NULL);
  self->length = length;
  self->type = type;
  return self;
}
static bool arr_type_is_equal(type_t self, type_t another) {
  return arr_type_get_length(self) == arr_type_get_length(another) &&
         type_is_equal(arr_type_get_type(self), arr_type_get_type(another));
}
static value_t arr_len(value_t self, context_t ctx) {
  type_t type = self->type;
  return create_comptime_u64(ctx, arr_type_get_length(type), false, NULL);
}
static value_t arr_get(value_t self, context_t ctx, value_t field) {
  type_t type = self->type;
  if (field->comptime) {
    uint64_t idx = 0;
    if (field->type->kind >= TYPE_KIND_I8 &&
        field->type->kind <= TYPE_KIND_I64) {
      int64_t val = integer_get_value(field);
      if (val < 0) {
        return create_error(ctx,
                            "array index %" PRIdPTR
                            " is before the beginning of the array",
                            val);
      }
      idx = val;
    } else if (field->type->kind >= TYPE_KIND_U8 &&
               field->type->kind <= TYPE_KIND_U64) {
      idx = unsigned_get_value(field);
    } else {
      return create_error(ctx, "array subscript is not an integer");
    }
    if (idx >= arr_type_get_length(type)) {
      return create_error(
          ctx, "array index %" PRIuPTR " is past the end of the array", idx);
    }
    if (self->comptime) {
      uint8_t *data = self->data;
      data = data + self->type->size * idx;
      return context_create_weak_value(ctx, arr_type_get_type(type), data,
                                       self->mut, NULL);
    } else {
      return context_create_value(ctx, arr_type_get_type(type), self->mut,
                                  NULL);
    }
  } else {
    if (self->comptime) {
      return create_error(ctx, "value is not comptime");
    }
    if (field->type->kind < TYPE_KIND_I8 && field->type->kind > TYPE_KIND_U64) {
      return create_error(ctx, "array subscript is not an integer");
    }
    return context_create_value(ctx, arr_type_get_type(type), self->mut, NULL);
  }
}
static value_t arr_set(value_t self, context_t ctx, value_t field,
                       value_t value) {
  value_t item = arr_get(self, ctx, field);
  if (item->type->kind == TYPE_KIND_ERROR) {
    return item;
  }
  return value_assigment(item, ctx, value);
}
static value_t arr_slice(value_t self, context_t ctx, value_t start,
                         value_t end) {
  type_t type = self->type;
  type_t base_type = arr_type_get_type(type);
  type_t slice_type = create_slice_type(ctx, base_type);
  if (!start->comptime || !end->comptime) {
    if (self->comptime) {
      return create_error(ctx, "value is not comptime");
    }
    return context_create_value(ctx, slice_type, self->mut, NULL);
  }
  size_t s = 0;
  size_t e = arr_type_get_length(type);
  if (start->type->kind >= TYPE_KIND_I8 && start->type->kind <= TYPE_KIND_I64) {
    int64_t val = integer_get_value(start);
    if (val < 0) {
      return create_error(
          ctx, "slice index %" PRIdPTR " is before the beginning of the slice",
          val);
    }
    s = val;
  } else if (start->type->kind >= TYPE_KIND_U8 &&
             start->type->kind <= TYPE_KIND_U64) {
    s = unsigned_get_value(start);
  } else if (start->type->kind != TYPE_KIND_VOID) {
    return create_error(ctx, "slice start is not an integer");
  }
  if (end->type->kind >= TYPE_KIND_I8 && end->type->kind <= TYPE_KIND_I64) {
    int64_t val = integer_get_value(end);
    if (val < 0) {
      return create_error(
          ctx, "slice index %" PRIdPTR " is before the beginning of the slice",
          val);
    }
    e = val;
  } else if (end->type->kind >= TYPE_KIND_U8 &&
             end->type->kind <= TYPE_KIND_U64) {
    e = unsigned_get_value(end);
  } else if (end->type->kind != TYPE_KIND_VOID) {
    return create_error(ctx, "slice start is not an integer");
  }
  if (s > e) {
    return create_error(ctx, "slice start %" PRIuPTR " >= end %" PRIuPTR, s, e);
  }
  size_t len = e - s;
  if (e > arr_type_get_length(type)) {
    return create_error(
        ctx, "slice end %" PRIuPTR " is past the end of the array", e);
  }
  if (self->comptime) {
    uint8_t *data = self->data;
    data = data + s * base_type->size;
    return create_comptime_slice(ctx, slice_type, data, s, len, self->mut,
                                 NULL);
  } else {
    return context_create_value(ctx, slice_type, self->mut, NULL);
  }
}
type_t create_arr_type(context_t ctx, type_t type, size_t length) {
  size_t len = snprintf(NULL, 0, "A%" PRIuPTR "%s", length, type->id);
  char id[len + 1];
  sprintf(id, "A%" PRIuPTR "%s", length, type->id);
  type_t atype = context_load_type(ctx, id);
  if (!atype) {
    len = snprintf(NULL, 0, "[%" PRIuPTR "]%s", length, type->name);
    char name[len + 1];
    sprintf(name, "[%" PRIuPTR "]%s", length, type->name);
    arr_meta_t meta = create_arr_meta(ctx->allocator, type, length);
    struct _type_operator_t opt = {
        .type_equal = arr_type_is_equal,
        .len = arr_len,
        .get = arr_get,
        .set = arr_set,
        .slice = arr_slice,
    };
    atype = create_type(ctx->allocator, TYPE_KIND_ARRAY, name, id,
                        type->size * length, type->align, &opt, meta);
    context_store_type(ctx, atype);
  }
  return atype;
}
type_t arr_type_get_type(type_t type) {
  arr_meta_t meta = type->meta;
  return meta->type;
}
size_t arr_type_get_length(type_t type) {
  arr_meta_t meta = type->meta;
  return meta->length;
}