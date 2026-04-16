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

value_t create_error(context_t ctx, const char *fmt, ...) {
  type_t type = context_get_error_type(ctx);
  allocator_t allocator = context_get_allocator(ctx);
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char message[len + 1];
  va_start(args, fmt);
  vsprintf(message, fmt, args);
  va_end(args);
  const char *str = context_create_cstring(ctx, message);
  return context_create_value(ctx, type, false, &str, NULL);
}

value_t create_compile_error(context_t ctx, ast_node_t node, const char *fmt,
                             ...) {
  type_t type = context_get_error_type(ctx);
  allocator_t allocator = context_get_allocator(ctx);
  char *line = location_get_line(node->loc, allocator);
  size_t len = strlen(line);
  char marks[len + 1];
  for (size_t idx = 0; idx < len + 1; idx++) {
    if (node->loc.begin.line == node->loc.end.line) {
      if (idx >= node->loc.begin.column - 1 && idx < node->loc.end.column - 1) {
        marks[idx] = '^';
      } else {
        marks[idx] = ' ';
      }
    } else {
      if (idx >= node->loc.begin.column - 1) {
        marks[idx] = '^';
      } else {
        marks[idx] = ' ';
      }
    }
  }
  marks[len] = 0;
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char message[len + 1];
  va_start(args, fmt);
  vsprintf(message, fmt, args);
  va_end(args);
  len = snprintf(NULL, 0, "%" PRIuPTR " |", node->loc.begin.line);
  char line_number[len + 1];
  sprintf(line_number, "%" PRIuPTR " |", node->loc.begin.line);
  size_t str_len =
      snprintf(NULL, 0, "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n%s%s\n%*s%s",
               node->loc.filename, node->loc.begin.line, node->loc.begin.column,
               message, line_number, line, (int)len, " ", marks);
  char msg[str_len + 1];
  sprintf(msg, "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n%s%s\n%*s%s",
          node->loc.filename, node->loc.begin.line, node->loc.begin.column,
          message, line_number, line, (int)len, " ", marks);
  const char *str = context_create_cstring(ctx, msg);
  allocator_free(allocator, line);
  return context_create_value(ctx, type, false, &str, NULL);
}
value_t convert_compile_error(context_t ctx, ast_node_t node, value_t err) {
  const char *message = *(const char **)value_get_data(err);
  return create_compile_error(ctx, node, "%s", message);
}
const char *error_get_message(value_t value) {
  const char *message = *(const char **)value_get_data(value);
  return message;
}