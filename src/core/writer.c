#include "core/writer.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include <sys/stat.h>
struct _line_t {
  string_t text;
  int indent;
};
typedef struct _line_t *line_t;
struct _line_init_t {
  string_t text;
  int indent;
};
typedef struct _line_init_t line_init_t;
static void line_init(line_t self, allocator_t allocator, line_init_t *init) {
  self->text = init->text;
  self->indent = init->indent;
}
static void line_dispose(line_t self, allocator_t allocator) {
  allocator_free(allocator, &self->text);
}
static void line_clone(line_t self, allocator_t allocator, line_t another) {
  self->text = value_clone(allocator, self->text);
  self->indent = another->indent;
}
static void line_move(line_t self, allocator_t allocator, line_t another) {
  self->text = another->text;
  self->indent = another->indent;
  another->text = allocator_create(allocator, &g_string_type, NULL);
  another->indent = 0;
}

static type_t g_line_type = {
    .name = "cubec.core.writer.line",
    .size = sizeof(struct _line_t),
    .init = (type_init_fn_t)line_init,
    .dispose = (type_dispose_fn_t)line_dispose,
    .clone = (type_clone_fn_t)line_clone,
    .move = (type_move_fn_t)line_move,
};
struct _writer_t {
  allocator_t allocator;
  vec_t lines;
};
static void writer_init(writer_t self, allocator_t allocator, void *init) {
  (void)(init);
  self->lines = allocator_create(allocator, &g_vec_type, NULL);
  self->allocator = allocator;
  line_init_t li = {
      .indent = 0,
      .text = allocator_create(allocator, &g_string_type, NULL),
  };
  line_t line = allocator_create(allocator, &g_line_type, &li);
  vec_push(self->lines, line);
}

static void writer_dispose(writer_t self, allocator_t allocator) {
  allocator_free(allocator, &self->lines);
}
static void writer_clone(writer_t self, allocator_t allocator,
                         writer_t another) {
  another->lines = value_clone(allocator, self->lines);
}
static void writer_move(writer_t self, allocator_t allocator,
                        writer_t another) {
  another->lines = self->lines;
  self->lines = allocator_create(allocator, &g_vec_type, NULL);
}

type_t g_writer_type = {
    .name = "cubec.core.writer",
    .size = sizeof(struct _writer_t),
    .init = (type_init_fn_t)writer_init,
    .dispose = (type_dispose_fn_t)writer_dispose,
    .clone = (type_clone_fn_t)writer_clone,
    .move = (type_move_fn_t)writer_move,
};

void writer_append(writer_t self, const char *str) {
  size_t size = vec_get_size(self->lines);
  line_t line = vec_get(self->lines, size - 1);
  string_concat(line->text, str);
}
void writer_newline(writer_t self, int32_t indent) {
  line_init_t li = {
      .indent = indent,
      .text = allocator_create(self->allocator, &g_string_type, NULL),
  };
  line_t line = allocator_create(self->allocator, &g_line_type, &li);
  vec_push(self->lines, line);
}

void writer_dedent_current_line(writer_t self, int32_t delta) {
  size_t size = vec_get_size(self->lines);
  line_t line = vec_get(self->lines, size - 1);
  line->indent += delta;
}
string_t writer_get_current_line(writer_t self) {
  return ((line_t)vec_get(self->lines, vec_get_size(self->lines) - 1))->text;
}
string_t writer_get_string(writer_t self) {
  string_t str = allocator_create(self->allocator, &g_string_type, NULL);
  size_t indent = 0;
  for (size_t idx = 0; idx < vec_get_size(self->lines); idx++) {
    line_t line = vec_get(self->lines, idx);
    indent += line->indent;
    if (string_get_length(line->text) > 0) {
      for (int32_t i = 0; i < indent; i++) {
        string_concat(str, "  ");
      }
    }
    string_concat(str, string_get(line->text));
    if (idx + 1 < vec_get_size(self->lines)) {
      string_concat(str, "\n");
    }
  }
  return str;
}