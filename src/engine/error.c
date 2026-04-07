#include "engine/error.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <corecrt_search.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void cubec_init_error_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {};
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_ERROR, sizeof(const char **),
                            sizeof(const char **), NULL, &opt, "error");
}
cubec_value_t cubec_create_error(cubec_context_t ctx, const char *fmt, ...) {
  cubec_value_t vtype = cubec_context_load(ctx, "error");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char message[len + 1];
  va_start(args, fmt);
  vsprintf(message, fmt, args);
  va_end(args);
  const char *str = cubec_context_create_cstring(ctx, message);
  return cubec_context_create_value(ctx, type, false, &str, NULL);
}

cubec_value_t cubec_create_compile_error(cubec_context_t ctx,
                                         cubec_ast_node_t node, const char *fmt,
                                         ...) {
  cubec_value_t vtype = cubec_context_load(ctx, "error");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *line = cubec_location_get_line(node->loc, allocator);
  size_t len = strlen(line);
  char marks[len + 1];
  for (size_t idx = 0; idx < len; idx++) {
    if (idx >= node->loc.begin.column && idx <= node->loc.end.column) {
      marks[idx] = '^';
    } else {
      marks[idx] = ' ';
    }
  }
  marks[len - 1] = 0;
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
               node->loc.filename, node->loc.begin.column, node->loc.begin.line,
               message, line_number, line, (int)len, " ", marks);
  char msg[str_len + 1];
  sprintf(msg, "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n%s%s\n%*s%s",
          node->loc.filename, node->loc.begin.column, node->loc.begin.line,
          message, line_number, line, (int)len, " ", marks);
  const char *str = cubec_context_create_cstring(ctx, msg);
  return cubec_context_create_value(ctx, type, false, &str, NULL);
}
cubec_value_t cubec_convert_compile_error(cubec_context_t ctx,
                                          cubec_ast_node_t node,
                                          cubec_value_t err) {
  const char *message = *(const char **)cubec_value_get_data(err);
  return cubec_create_compile_error(ctx, node, "%s", message);
}