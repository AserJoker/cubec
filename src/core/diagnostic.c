#include "core/diagnostic.h"
#include "engine/context.h"
#include "engine/module.h"
#include <stdarg.h>
#include <string.h>

/* ===== diagnostic_list ===== */

struct _diagnostic_list_t {
  allocator_t allocator;
  vec_t diagnostics; /* vec of struct diagnostic* */
  FILE *output;
  size_t error_count;
};

static void _diagnostic_list_init(diagnostic_list_t self, allocator_t allocator,
                                  diagnostic_list_init_t *init) {
  self->allocator = allocator;
  vec_init_t vec_init = {.auto_dispose = true};
  self->diagnostics =
      (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  self->output = (init && init->output) ? init->output : stderr;
  self->error_count = 0;
}

static void _diagnostic_list_dispose(diagnostic_list_t self,
                                     allocator_t allocator) {
  size_t size = vec_get_size(self->diagnostics);
  for (size_t i = 0; i < size; i++) {
    struct diagnostic *d = (struct diagnostic *)vec_get(self->diagnostics, i);
    allocator_free(allocator, &d->notes);
  }
  allocator_free(allocator, &self->diagnostics);
}

static void _diagnostic_list_clone(diagnostic_list_t self, allocator_t allocator,
                                   diagnostic_list_t another) {
  self->allocator = allocator;
  vec_init_t vec_init = {.auto_dispose = true};
  self->diagnostics =
      (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  self->output = another->output;
  self->error_count = another->error_count;
  size_t size = vec_get_size(another->diagnostics);
  for (size_t i = 0; i < size; i++) {
    struct diagnostic *src = (struct diagnostic *)vec_get(another->diagnostics, i);
    struct diagnostic *dst = (struct diagnostic *)allocator_alloc(allocator, sizeof(struct diagnostic));
    *dst = *src;
    dst->notes = (vec_t)alloc_clone(allocator, src->notes);
    vec_push(self->diagnostics, dst);
  }
}

type_t g_diagnostic_list_type = {
    .size = sizeof(struct _diagnostic_list_t),
    .name = "cubec.core.diagnostic_list",
    .init = (type_init_fn_t)_diagnostic_list_init,
    .dispose = (type_dispose_fn_t)_diagnostic_list_dispose,
    .clone = (type_clone_fn_t)_diagnostic_list_clone,
};

/* ===== public API ===== */

size_t diagnostic_list_get_size(diagnostic_list_t self) {
  return vec_get_size(self->diagnostics);
}

struct diagnostic *diagnostic_list_get(diagnostic_list_t self, size_t idx) {
  if (idx >= vec_get_size(self->diagnostics)) return NULL;
  return (struct diagnostic *)vec_get(self->diagnostics, idx);
}

size_t diagnostic_list_get_error_count(diagnostic_list_t self) {
  return self->error_count;
}

struct diagnostic *diagnostic_list_push(diagnostic_list_t self,
                                        enum diagnostic_severity severity,
                                        location_t primary,
                                        const char *fmt, ...) {
  struct diagnostic *d = (struct diagnostic *)allocator_alloc(
      self->allocator, sizeof(struct diagnostic));
  d->severity = severity;
  d->primary = primary;

  va_list args;
  va_start(args, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt, args);
  va_end(args);

  vec_init_t notes_init = {.auto_dispose = true};
  d->notes = (vec_t)allocator_create(self->allocator, &g_vec_type, &notes_init);

  vec_push(self->diagnostics, d);

  if (severity == DIAGNOSTIC_ERROR) {
    self->error_count++;
  }

  return d;
}

void diagnostic_list_push_note(diagnostic_list_t self, location_t location,
                               const char *fmt, ...) {
  size_t size = vec_get_size(self->diagnostics);
  if (size == 0) {
    return;
  }

  struct diagnostic *d =
      (struct diagnostic *)vec_get(self->diagnostics, size - 1);

  struct diagnostic_note *note = (struct diagnostic_note *)allocator_alloc(
      self->allocator, sizeof(struct diagnostic_note));
  note->location = location;

  va_list args;
  va_start(args, fmt);
  vsnprintf(note->message, sizeof(note->message), fmt, args);
  va_end(args);

  vec_push(d->notes, note);
}

/* ===== formatting helpers ===== */

static const char *severity_str(enum diagnostic_severity severity) {
  switch (severity) {
  case DIAGNOSTIC_ERROR:
    return "error";
  case DIAGNOSTIC_WARNING:
    return "warning";
  case DIAGNOSTIC_NOTE:
    return "note";
  }
  return "unknown";
}

static void format_location(FILE *out, location_t *loc) {
  if (loc->filename) {
    fprintf(out, "  --> %s:%zu:%zu", loc->filename, loc->begin.line + 1,
            loc->begin.column + 1);
  }
}

static void emit_diagnostic(FILE *out, struct diagnostic *d,
                            struct context *ctx) {
  fprintf(out, "%s: %s\n", severity_str(d->severity), d->message);

  format_location(out, &d->primary);
  fprintf(out, "\n");

  if (ctx && d->primary.filename) {
    module_t mod = context_get_module(ctx, d->primary.filename);
    if (mod) {
      const char *src = mod->source;
      if (src) {
        /* Compute source line from the module's source text */
        size_t line = d->primary.begin.line + 1;
        size_t line_start = 0;
        size_t current_line = 1;
        size_t src_len = strlen(src);

        for (size_t i = 0; i < src_len && current_line < line; i++) {
          if (src[i] == '\n') {
            current_line++;
            line_start = i + 1;
          }
        }

        size_t line_end = line_start;
        while (line_end < src_len && src[line_end] != '\n' && src[line_end] != '\r') {
          line_end++;
        }

        /* Count total lines for width calculation */
        size_t line_count = 1;
        for (size_t i = 0; i < src_len; i++) {
          if (src[i] == '\n') line_count++;
        }

        int width = 1;
        size_t tmp = line_count;
        while (tmp >= 10) {
          tmp /= 10;
          width++;
        }

        size_t line_len = line_end - line_start;
        if (line_len >= 4096) line_len = 4095;
        char line_buf[4096];
        memcpy(line_buf, src + line_start, line_len);
        line_buf[line_len] = '\0';

        fprintf(out, " %*s |\n", width, "");
        fprintf(out, " %*zu | %s\n", width, line, line_buf);

        fprintf(out, " %*s | ", width, "");
        size_t col = d->primary.begin.column + 1;
        size_t span_len = 1;
        if (d->primary.end.offset > d->primary.begin.offset) {
          span_len = (size_t)(d->primary.end.offset - d->primary.begin.offset);
        }
        for (size_t i = 1; i < col; i++) {
          fprintf(out, " ");
        }
        for (size_t i = 0; i < span_len; i++) {
          fprintf(out, "^");
        }
        fprintf(out, "\n");
      }
    }
  }

  size_t note_count = vec_get_size(d->notes);
  for (size_t i = 0; i < note_count; i++) {
    struct diagnostic_note *note =
        (struct diagnostic_note *)vec_get(d->notes, i);
    fprintf(out, "  = %s: %s\n", severity_str(DIAGNOSTIC_NOTE), note->message);
    if (note->location.filename) {
      fprintf(out, "    --> %s:%zu:%zu\n", note->location.filename,
              note->location.begin.line + 1, note->location.begin.column + 1);
    }
  }

  fprintf(out, "\n");
}

void diagnostic_list_emit(diagnostic_list_t self, struct context *ctx) {
  size_t size = vec_get_size(self->diagnostics);
  for (size_t i = 0; i < size; i++) {
    struct diagnostic *d = (struct diagnostic *)vec_get(self->diagnostics, i);
    emit_diagnostic(self->output, d, ctx);
  }
}

void diagnostic_list_clear(diagnostic_list_t self) {
  size_t size = vec_get_size(self->diagnostics);
  for (size_t i = 0; i < size; i++) {
    struct diagnostic *d = (struct diagnostic *)vec_get(self->diagnostics, i);
    allocator_free(self->allocator, &d->notes);
  }
  vec_resize(self->diagnostics, 0);
  self->error_count = 0;
}
