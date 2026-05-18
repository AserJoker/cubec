#include "engine/str.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
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
void init_str_type(context_t ctx) {
  struct _type_operator_t opt = {
    .opt_eq = str_eq,
    .opt_ne = str_ne,
  };
  type_t type =
      create_type(ctx->allocator, TYPE_KIND_STR, "str", "str",
                  sizeof(const char *), sizeof(const char *), &opt, NULL);
  context_store_type(ctx, type);
}

value_t create_comptime_str(context_t ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *str = context_create_string(ctx, msg);
  type_t type = context_load_type(ctx, "str");
  return context_create_comptime_value(ctx, type, &str, false, NULL);
}