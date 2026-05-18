#include "engine/error.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdarg.h>
#include <stdio.h>

void init_error_type(context_t ctx) {
  type_t error_type =
      create_type(ctx->allocator, TYPE_KIND_ERROR, "error", "error",
                  sizeof(const char *), sizeof(const char *), NULL, NULL);
  context_store_type(ctx, error_type);

  type_t format_error_type = create_type(
      ctx->allocator, TYPE_KIND_ERROR, "format_error", "format_error",
      sizeof(const char *), sizeof(const char *), NULL, NULL);
  context_store_type(ctx, format_error_type);
}

value_t create_error(context_t ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *str = context_create_string(ctx, msg);
  type_t type = context_load_type(ctx, "error");
  return context_create_comptime_value(ctx, type, &str, false, NULL);
}