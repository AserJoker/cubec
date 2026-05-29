#include "engine/error.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

void init_error_type(context_t ctx) {
  type_t error_type = create_type(ctx->allocator, TYPE_KIND_ERROR, "error",
                                  "error", sizeof(struct _error_t),
                                  sizeof(struct _error_t), NULL, NULL, true);
  context_store_type(ctx, error_type);
}

value_t create_error(context_t ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len + 1];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *str = context_create_string(ctx, msg);
  struct _error_t err = {
      .message = str,
      .loc = {},
  };
  type_t type = context_load_type(ctx, "error");
  return context_create_comptime_value(ctx, type, &err, false, NULL);
}
value_t create_comptime_error(context_t ctx, location_t loc, const char *fmt,
                              ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len + 1];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *str = context_create_string(ctx, msg);
  struct _error_t err = {
      .message = str,
      .loc = loc,
  };
  type_t type = context_load_type(ctx, "error");
  return context_create_comptime_value(ctx, type, &err, false, NULL);
}
value_t convert_comptime_error(context_t ctx, location_t loc, value_t error) {
  error_t data = error->data;
  struct _error_t err = {
      .message = data->message,
      .loc = data->loc.filename ? data->loc : loc,
  };
  type_t type = context_load_type(ctx, "error");
  return context_create_comptime_value(ctx, type, &err, false, NULL);
}
char *error_format(allocator_t allocator, value_t error) {
  error_t err = error->data;
  if (err->loc.end.offset == 0) {
    return create_cstring(allocator, err->message);
  }
  size_t line_number = err->loc.end.line + 1;
  size_t column_number = err->loc.end.column + 1;
  char *line_data = location_get_line(err->loc, allocator);
  size_t len = snprintf(NULL, 0, "%" PRIuPTR " |%s", line_number, line_data);
  char line[len + 1];
  sprintf(line, "%" PRIuPTR " |%s", line_number, line_data);
  allocator_free(allocator, line_data);
  char mask[len + 1];
  size_t prefix_len = snprintf(NULL, 0, "%" PRIuPTR " |", line_number);
  size_t idx = 0;
  for (idx = 0; idx < len; idx++) {
    if (err->loc.end.line == err->loc.begin.line) {
      if (idx < err->loc.begin.column + prefix_len ||
          idx >= column_number + prefix_len - 1) {
        mask[idx] = ' ';
      } else {
        mask[idx] = '^';
      }
    } else {
      if (idx > column_number + prefix_len) {
        mask[idx] = ' ';
      } else {
        mask[idx] = '^';
      }
    }
  }
  mask[idx] = 0;
  len = snprintf(NULL, 0, "%s:%" PRIuPTR ":%" PRIuPTR ":%s\n%s\n%s",
                 err->loc.filename, line_number, column_number, err->message,
                 line, mask);
  char *message = allocator_alloc(allocator, len + 1, NULL);
  sprintf(message, "%s:%" PRIuPTR ":%" PRIuPTR ":%s\n%s\n%s", err->loc.filename,
          line_number, column_number, err->message, line, mask);
  return message;
}