#include "core/stream.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
struct _line_t {
  size_t indent;
  string_t buffer;
};
typedef struct _line_t *line_t;
static void line_dispose(line_t line, allocator_t allocator) {
  allocator_free(allocator, line->buffer);
}
static line_t create_line(allocator_t allocator, size_t indent) {
  line_t self = allocator_alloc(allocator, sizeof(struct _line_t),
                                (dispose_fn_t)line_dispose);
  self->buffer = create_string(allocator, NULL);
  self->indent = indent;
  return self;
}
struct _stream_t {
  allocator_t allocator;
  size_t indent;
  size_t base_indent;
  array_t lines;
  line_t line;
};
static void stream_dispose(stream_t self, allocator_t allocator) {
  allocator_free(allocator, self->lines);
}
stream_t create_stream(allocator_t allocator) {
  stream_t self = allocator_alloc(allocator, sizeof(struct _stream_t),
                                  (dispose_fn_t)stream_dispose);
  self->allocator = allocator;
  self->base_indent = 2;
  self->indent = 0;
  array_initialize_t initialize = {
      .autofree = true,
  };
  self->lines = create_array(allocator, &initialize);
  self->line = create_line(allocator, 0);
  array_push(self->lines, self->line);
  return self;
}
void stream_write(stream_t stream, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char str[len + 1];
  va_start(args, fmt);
  vsprintf(str, fmt, args);
  va_end(args);
  string_concat(stream->line->buffer, stream->allocator, str);
}
void stream_write_location(stream_t stream, location_t loc) {
  string_concat_location(stream->line->buffer, stream->allocator, loc);
}
void stream_newline(stream_t stream) {
  stream->line = create_line(stream->allocator, stream->indent);
  array_push(stream->lines, stream->line);
}
void stream_popline(stream_t stream) { array_pop(stream->lines); }

void stream_inc_indent(stream_t stream) { stream->indent++; }
void stream_dec_indent(stream_t stream) { stream->indent--; }
void stream_set_indent(stream_t stream, size_t indent) {
  stream->indent = indent;
}
size_t stream_get_indent(stream_t stream) { return stream->indent; }
void stream_set_base_indent(stream_t stream, size_t base_indent) {
  stream->base_indent = base_indent;
}
size_t stream_get_base_indent(stream_t stream) { return stream->base_indent; }

string_t stream_get_string(stream_t stream) {
  string_t str = create_string(stream->allocator, NULL);
  for (size_t idx = 0; idx < array_get_size(stream->lines); idx++) {
    line_t line = array_get(stream->lines, idx);
    char buf[line->indent * stream->base_indent + 1];
    memset(buf, ' ', line->indent * stream->base_indent);
    buf[line->indent * stream->base_indent] = 0;
    string_concat(str, stream->allocator, buf);
    const char *s = string_get(line->buffer);
    string_concat(str, stream->allocator, s);
    string_concat(str, stream->allocator, "\n");
  }
  return str;
}
void stream_merge(stream_t stream, stream_t another) {
  for (size_t idx = 0; idx < array_get_size(another->lines); idx++) {
    line_t line = array_get(another->lines, idx);
    line_t newline = create_line(stream->allocator, 0);
    newline->indent = line->indent;
    const char *str = string_get(line->buffer);
    string_concat(newline->buffer, stream->allocator, str);
    array_push(stream->lines, newline);
  }
  stream->line = create_line(stream->allocator, 0);
  array_push(stream->lines, stream->line);
}