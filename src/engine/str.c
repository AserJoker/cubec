#include "engine/str.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static value_t str_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_STR) {
    if (self->comptime && another->comptime) {
      const char *lvalue = *(const char **)self->data;
      const char *rvalue = *(const char **)another->data;
      return create_comptime_bool(ctx, strcmp(lvalue, rvalue) == 0, false,
                                  NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t str_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_STR) {
    if (self->comptime && another->comptime) {
      const char *lvalue = *(const char **)self->data;
      const char *rvalue = *(const char **)another->data;
      return create_comptime_bool(ctx, strcmp(lvalue, rvalue) != 0, false,
                                  NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t str_slice(value_t self, context_t ctx, value_t start,
                         value_t end) {
  type_t base_type = context_load_type(ctx, "u8");
  type_t slice_type = create_slice_type(ctx, base_type);
  if (!start->comptime || !end->comptime) {
    if (self->comptime) {
      return create_error(ctx, "value is not comptime");
    }
    return context_create_value(ctx, slice_type, false, NULL);
  }
  if (!self->comptime) {
    return context_create_value(ctx, slice_type, false, NULL);
  }
  const char *data = *(const char **)self->data;
  size_t length = strlen(data);
  size_t s = 0;
  size_t e = length;
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
  if (e > length) {
    return create_error(
        ctx, "slice end %" PRIuPTR " is past the end of the array", e);
  }
  if (self->comptime) {
    uint8_t *data = self->data;
    data = data + s * base_type->size;
    return create_comptime_slice(ctx, slice_type, data, s, len, false, NULL);
  } else {
    return context_create_value(ctx, slice_type, false, NULL);
  }
}
static value_t str_length(value_t self, context_t ctx) {
  if (self->comptime) {
    const char *data = *(const char **)self->data;
    size_t length = strlen(data);
    return create_comptime_u64(ctx, length, false, NULL);
  }
  return create_u64(ctx, false, NULL);
}
void init_str_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_eq = str_eq,
      .opt_ne = str_ne,
      .slice = str_slice,
      .len = str_length,
  };
  type_t type = create_type(ctx->allocator, TYPE_KIND_STR, "str", "str",
                            sizeof(const char *), sizeof(const char *), &opt,
                            NULL, false);
  context_store_type(ctx, type);
  create_type_value(ctx, type, false, "str");
}

value_t create_comptime_str(context_t ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len + 1];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *str = context_create_string(ctx, msg);
  type_t type = context_load_type(ctx, "str");
  return context_create_comptime_value(ctx, type, &str, false, NULL);
}