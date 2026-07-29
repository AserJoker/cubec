#include "engine/source.h"
#include "core/allocator.h"
#include <string.h>
#include <threads.h>

/* ===== source_entry helpers ===== */

static void compute_line_offsets(struct source_entry *entry,
                                 allocator_t allocator) {
  vec_resize(entry->line_offsets, 0);
  const char *data = string_get(entry->content);
  size_t len = string_get_length(entry->content);

  /* line 1 always starts at offset 0 */
  size_t offset = 0;
  vec_push(entry->line_offsets, (void *)offset);

  for (size_t i = 0; i < len; i++) {
    if (data[i] == '\n') {
      offset = i + 1;
      vec_push(entry->line_offsets, (void *)offset);
    }
  }
}

const char *source_entry_get_line(struct source_entry *entry, size_t line) {
  if (line == 0 || line > vec_get_size(entry->line_offsets)) {
    return "";
  }
  size_t start = (size_t)vec_get(entry->line_offsets, line - 1);
  const char *data = string_get(entry->content);
  size_t len = string_get_length(entry->content);

  /* find end of line (excluding newline) */
  size_t end = start;
  while (end < len && data[end] != '\n' && data[end] != '\r') {
    end++;
  }

  /* temporarily null-terminate the line by writing into the string buffer.
   * This is safe because string_t's internal buffer always has extra capacity
   * beyond len, and we restore the character after use.
   * However, we cannot modify the buffer since string_t is supposed to be
   * immutable after set. Instead, we use a thread-local static buffer. */
  static thread_local char line_buf[4096];
  size_t line_len = end - start;
  if (line_len >= sizeof(line_buf)) {
    line_len = sizeof(line_buf) - 1;
  }
  memcpy(line_buf, data + start, line_len);
  line_buf[line_len] = '\0';
  return line_buf;
}

size_t source_entry_get_line_count(struct source_entry *entry) {
  return vec_get_size(entry->line_offsets);
}

/* ===== source_cache ===== */

struct _source_cache_t {
  allocator_t allocator;
  strmap_t entries; /* filename -> source_entry* */
};

static void _source_cache_init(source_cache_t self, allocator_t allocator,
                               void *arg) {
  (void)arg;
  self->allocator = allocator;
  strmap_init_t init = {.value_auto_dispose = true};
  self->entries =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &init);
}

static void _source_cache_dispose(source_cache_t self, allocator_t allocator) {
  /* strmap auto_dispose will free each source_entry */
  allocator_free(allocator, &self->entries);
}

type_t g_source_cache_type = {
    .size = sizeof(struct _source_cache_t),
    .name = "cubec.engine.source_cache",
    .init = (type_init_fn_t)_source_cache_init,
    .dispose = (type_dispose_fn_t)_source_cache_dispose,
};

/* ===== source_entry as object ===== */

static void _source_entry_init(void *self, allocator_t allocator, void *arg) {
  struct source_entry *entry = (struct source_entry *)self;
  (void)arg;
  string_init_t str_init = {.str = NULL};
  entry->content =
      (string_t)allocator_create(allocator, &g_string_type, &str_init);
  vec_init_t vec_init = {.auto_dispose = false};
  entry->line_offsets =
      (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
}

static void _source_entry_dispose(void *self, allocator_t allocator) {
  struct source_entry *entry = (struct source_entry *)self;
  allocator_free(allocator, &entry->content);
  allocator_free(allocator, &entry->line_offsets);
}

static type_t g_source_entry_type = {
    .size = sizeof(struct source_entry),
    .name = "cubec.engine.source_entry",
    .init = (type_init_fn_t)_source_entry_init,
    .dispose = (type_dispose_fn_t)_source_entry_dispose,
};

struct source_entry *source_cache_load(source_cache_t self,
                                       const char *filename,
                                       const char *content,
                                       bool take_ownership) {
  /* check if already loaded */
  struct source_entry *existing =
      (struct source_entry *)strmap_find(self->entries, filename);
  if (existing) {
    return existing;
  }

  /* create new entry */
  struct source_entry *entry = (struct source_entry *)allocator_create(
      self->allocator, &g_source_entry_type, NULL);

  if (take_ownership) {
    /* move content into the string_t */
    string_set(entry->content, content);
  } else {
    string_set(entry->content, content);
  }

  compute_line_offsets(entry, self->allocator);

  strmap_insert(self->entries, filename, entry);
  return entry;
}

struct source_entry *source_cache_find(source_cache_t self,
                                       const char *filename) {
  return (struct source_entry *)strmap_find(self->entries, filename);
}
