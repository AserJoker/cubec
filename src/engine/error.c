#include "engine/error.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void error_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_t error_t =
      create_type(allocator, TYPE_KIND_ERROR, sizeof(const char *),
                  sizeof(const char *), "error", "error", NULL, NULL);
  context_store_type(ctx, error_t);
  create_type_value(ctx, error_t, false, true, "error");
}
value_t create_error(context_t ctx, const char *fmt, ...) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t vtype = context_load(ctx, "error");
  type_t type = *(type_t *)value_get_data(vtype);
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len + 1];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  const char *message = context_create_cstring(ctx, msg);
  return context_create_value(ctx, type, &message, false, true, NULL);
}
value_t create_compile_error(context_t ctx, ast_node_t node, const char *fmt,
                             ...) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t vtype = context_load(ctx, "error");
  type_t type = *(type_t *)value_get_data(vtype);
  size_t len = snprintf(NULL, 0, "%" PRIuPTR " |", node->loc.begin.line + 1);
  char prefix[len + 1];
  sprintf(prefix, "%" PRIuPTR " |", node->loc.begin.line + 1);
  size_t prefix_len = strlen(prefix);
  char *line = location_get_line(node->loc, allocator);
  len = strlen(line);
  char mask[len + prefix_len + 1];
  if (node->loc.begin.line != node->loc.end.line) {
    for (size_t idx = 0; idx < len; idx++) {
      if (idx < node->loc.begin.column - 1) {
        mask[idx + prefix_len] = ' ';
      } else {
        mask[idx + prefix_len] = '^';
      }
    }
  } else {
    for (size_t idx = 0; idx < len; idx++) {
      if (idx >= node->loc.begin.column - 1 && idx < node->loc.end.column - 1) {
        mask[idx + prefix_len] = '^';
      } else {
        mask[idx + prefix_len] = ' ';
      }
    }
  }
  for (size_t idx = 0; idx < prefix_len; idx++) {
    mask[idx] = ' ';
  }
  mask[len + prefix_len] = 0;

  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char msg[len + 1];
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  len = snprintf(NULL, 0, "%s:%" PRIuPTR ":%" PRIuPTR " error: %s\n%s%s\n%s",
                 node->loc.filename, node->loc.end.line + 1,
                 node->loc.end.column, msg, prefix, line, mask);
  char message[len + 1];
  sprintf(message, "%s:%" PRIuPTR ":%" PRIuPTR " error: %s\n%s%s\n%s",
          node->loc.filename, node->loc.end.line + 1, node->loc.end.column, msg,
          prefix, line, mask);
  allocator_free(allocator, line);
  const char *str = context_create_cstring(ctx, message);
  return context_create_value(ctx, type, &str, false, true, NULL);
}
value_t convert_compile_error(context_t ctx, ast_node_t node, value_t err) {
  const char *message = error_get_message(err);
  return create_compile_error(ctx, node, message);
}
const char *error_get_message(value_t self) {
  return *(const char **)value_get_data(self);
}