#include "engine/slice.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
typedef struct _slice_meta_t *slice_meta_t;
struct _slice_meta_t {
  type_t type;
};
static slice_meta_t create_slice_meta(allocator_t allocator, type_t type) {
  slice_meta_t self =
      allocator_alloc(allocator, sizeof(struct _slice_meta_t), NULL);
  self->type = type;
  return self;
}
static bool slice_type_is_equal(type_t self, type_t another) {
  if (another->kind == TYPE_KIND_SLICE) {
    return type_is_equal(slice_type_get_type(self),
                         slice_type_get_type(another));
  }
  return false;
}
static value_t slice_len(value_t self, context_t ctx) {
  if (self->comptime) {
    slice_data_t data = self->data;
    return create_comptime_u64(ctx, data->length, false, NULL);
  }
  return create_u64(ctx, false, NULL);
}
static value_t slice_get(value_t self, context_t ctx, value_t field) {
  type_t type = self->type;
  if (field->comptime && self->comptime) {
    uint64_t idx = 0;
    slice_data_t data = self->data;
    if (field->type->kind >= TYPE_KIND_I8 &&
        field->type->kind <= TYPE_KIND_I64) {
      int64_t val = integer_get_value(field);
      if (val < 0) {
        return create_error(ctx,
                            "slice index %" PRIdPTR
                            " is before the beginning of the slice",
                            val);
      }
      idx = val;
    } else if (field->type->kind >= TYPE_KIND_U8 &&
               field->type->kind <= TYPE_KIND_U64) {
      idx = unsigned_get_value(field);
    } else {
      return create_error(ctx, "slice subscript is not an integer");
    }
    if (idx >= data->length) {
      return create_error(
          ctx, "slice index %" PRIuPTR " is past the end of the slice", idx);
    }
    uint8_t *buf = self->data;
    buf = buf + self->type->size * (idx + data->offset);
    return context_create_weak_value(ctx, slice_type_get_type(type), data,
                                     self->mut, NULL);
  } else {
    if (field->type->kind < TYPE_KIND_I8 && field->type->kind > TYPE_KIND_U64) {
      return create_error(ctx, "slice subscript is not an integer");
    }
    return context_create_value(ctx, slice_type_get_type(type), self->mut,
                                NULL);
  }
}
static value_t slice_slice(value_t self, context_t ctx, value_t start,
                           value_t end) {
  type_t type = self->type;
  if (!start->comptime || !end->comptime) {
    if (self->comptime) {
      return create_error(ctx, "value is not comptime");
    }
    return context_create_value(ctx, type, self->mut, NULL);
  }
  slice_data_t data = self->data;
  size_t s = 0;
  size_t e = data->length;
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
  return create_comptime_slice(ctx, type, data->data, data->offset + s, len,
                               self->mut, NULL);
}
static value_t slice_set(value_t self, context_t ctx, value_t field,
                         value_t value) {
  value_t item = slice_get(self, ctx, field);
  if (item->type == TYPE_KIND_ERROR) {
    return item;
  }
  return value_assigment(item, ctx, value);
}
type_t create_slice_type(context_t ctx, type_t type) {
  size_t len = snprintf(NULL, 0, "S%s", type->id);
  char id[len];
  sprintf(id, "S%s", type->id);
  type_t stype = context_load_type(ctx, id);
  if (!stype) {
    size_t len = snprintf(NULL, 0, "S%s", type->name);
    char name[len];
    sprintf(name, "[]%s", type->name);
    slice_meta_t meta = create_slice_meta(ctx->allocator, type);
    struct _type_operator_t opt = {
        .type_equal = slice_type_is_equal,
        .len = slice_len,
        .get = slice_get,
        .set = slice_set,
        .slice = slice_slice,
    };
    stype = create_type(ctx->allocator, TYPE_KIND_SLICE, name, id,
                        sizeof(struct _slice_data_t),
                        alignof(struct _slice_data_t), &opt, meta);
    context_store_type(ctx, stype);
  }
  return stype;
}
type_t slice_type_get_type(type_t type) {
  slice_meta_t meta = type->meta;
  return meta->type;
}
value_t create_comptime_slice(context_t ctx, type_t type, void *data,
                              size_t offset, size_t length, bool mut,
                              const char *name) {
  struct _slice_data_t d = {
      .data = data,
      .offset = offset,
      .length = length,
  };
  return context_create_comptime_value(ctx, type, &d, mut, name);
}