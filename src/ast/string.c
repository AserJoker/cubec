#include "core/string.h"
#include "core/allocator.h"
#include <string.h>
struct _cubec_string_t {
  char *data;
  size_t len;
  size_t capacity;
};
static void cubec_string_dispose(cubec_string_t self,
                                 cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_string_t cubec_create_string(cubec_allocator_t allocator,
                                   cubec_string_initialize_t *initialize) {
  cubec_string_t string =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_string_t),
                            (cubec_dispose_fn_t)cubec_string_dispose);
  if (initialize) {
    if (initialize->source) {
      string->len = strlen(initialize->source);
    } else {
      string->len = 0;
    }
    string->capacity = string->len + 1;
    if (string->capacity < 16) {
      string->capacity = 16;
    }
    string->data = cubec_allocator_alloc(allocator, string->capacity, NULL);
    if (initialize->source) {
      memcpy(string->data, initialize->source, string->len + 1);
    } else {
      string->data[0] = 0;
    }
  } else {
    string->capacity = 16;
    string->len = 0;
    string->data = cubec_allocator_alloc(allocator, string->capacity, NULL);
    string->data[0] = 0;
  }
  return string;
}

const char *cubec_string_get(cubec_string_t self) { return self->data; }

size_t cubec_string_len(cubec_string_t self) { return self->len; }

cubec_string_t cubec_string_set(cubec_string_t self,
                                cubec_allocator_t allocator,
                                const char *source) {
  size_t len = strlen(source);
  if (len >= self->capacity) {
    self->capacity = len + 1;
    char *str = cubec_allocator_alloc(allocator, self->capacity, NULL);
    str[0] = 0;
    cubec_allocator_free(allocator, self->data);
    self->data = str;
  }
  memcpy(self->data, source, len + 1);
  self->len = len;
  return self;
}
cubec_string_t cubec_string_concat(cubec_string_t self,
                                   cubec_allocator_t allocator,
                                   const char *source) {
  size_t len = strlen(source);
  if (len + self->len + 1 >= self->capacity) {
    self->capacity = len + self->len + 1;
    char *str = cubec_allocator_alloc(allocator, self->capacity, NULL);
    memcpy(str, self->data, self->len + 1);
    cubec_allocator_free(allocator, self->data);
    self->data = str;
  }
  memcpy(&self->data[self->len], source, len + 1);
  self->len += len;
  return self;
}
int cubec_string_compare(cubec_string_t self, const char *source) {
  return strcmp(self->data, source);
}