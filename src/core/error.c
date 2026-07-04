#include "core/error.h"
#include "core/allocator.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
thread_local error_t *g_error = NULL;
void throw_error(const char *filename, const char *funcname, size_t line,
                 const char *fmt, ...) {
  static error_t error;
  va_list args;
  va_start(args, fmt);
  vsnprintf(error.message, sizeof(error.message), fmt, args);
  va_end(args);
  error.stacktop = 1;
  error.stack[0].filename = filename;
  error.stack[0].funcname = funcname;
  error.stack[0].line = line;
  g_error = &error;
}
void error_push(const char *filename, const char *funcname, size_t line) {
  if (g_error == NULL) {
    return;
  }
  if (g_error->stacktop == 64) {
    return;
  }
  error_frame_t *frame = &g_error->stack[g_error->stacktop];
  frame->filename = filename;
  frame->funcname = funcname;
  frame->line = line;
  g_error->stacktop++;
}

void error_clear() { g_error = NULL; }

char *error_to_string(error_t *error, allocator_t allocator) {
  size_t len = strlen(error->message) + 8;
  for (size_t idx = 0; idx < error->stacktop; idx++) {
    error_frame_t *frame = &error->stack[idx];
    len += snprintf(NULL, 0, "  %s:%" PRIuPTR " (%s)\n", frame->filename,
                    frame->line, frame->funcname);
  }
  char *str = allocator ? allocator_alloc(allocator, len + 1) : malloc(len + 1);
  size_t offset = sprintf(str, "%s\nat\n", error->message);
  for (size_t idx = 0; idx < error->stacktop; idx++) {
    error_frame_t *frame = &error->stack[idx];
    offset += sprintf(&str[offset], "  %s:%" PRIuPTR " (%s)\n", frame->filename,
                      frame->line, frame->funcname);
  }
  return str;
}