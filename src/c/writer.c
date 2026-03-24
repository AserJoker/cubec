#include "c/writer.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

cubec_value_t cubec_c_create_error(cubec_context_t self, cubec_ast_node_t node,
                                   const char *filename, const char *fmt, ...) {
  char num[16];
  sprintf(num, "%" PRIuPTR " | ", node->loc.begin.line);
  char *line = cubec_location_get_line(node->loc, self->allocator);
  size_t len = strlen(line);
  size_t column = node->loc.begin.column - 1;
  len += strlen(num);
  char marks[len + 1];
  memset(marks, 0, len + 1);
  for (size_t id = 0; id < len; id++) {
    if (id < column + strlen(num)) {
      marks[id] = ' ';
    } else {
      marks[id] = '^';
    }
  }
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  char *msg = cubec_allocator_alloc(self->allocator, len + 1, NULL);
  va_end(args);
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  cubec_value_t err =
      cubec_context_create_error(self,
                                 "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n"
                                 "%s%s\n"
                                 "%s\n",
                                 filename, node->loc.begin.line,
                                 node->loc.begin.column, msg, num, line, marks);
  cubec_allocator_free(self->allocator, line);
  cubec_allocator_free(self->allocator, msg);
  return err;
}