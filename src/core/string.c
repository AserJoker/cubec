#include "core/string.h"
#include "core/allocator.h"
#include <string.h>
struct _string_t {
  allocator_t allocator;
  size_t size;
  size_t capacity;
  char *data;
};

static void _string_init(string_t self, allocator_t allocator,
                         string_init_t *pstr) {
  const char *str = NULL;
  if (pstr) {
    str = pstr->str;
  }
  self->allocator = allocator;
  self->size = str ? strlen(str) + 1 : 1;
  self->capacity = 1;
  while (self->capacity < self->size) {
    self->capacity *= 2;
  }
  self->data = allocator_alloc(allocator, self->capacity);
  if (str) {
    memcpy(self->data, str, self->size);
  } else {
    self->data[0] = '\0';
  }
}
static void _string_clone(string_t self, allocator_t allocator,
                          string_t another) {
  self->size = another->size;
  self->capacity = another->capacity;
  self->allocator = another->allocator;
  self->data = allocator_alloc(allocator, self->capacity);
  memcpy(self->data, another->data, another->size);
}
static void _string_move(string_t self, allocator_t allocator,
                         string_t another) {
  self->size = another->size;
  self->capacity = another->capacity;
  self->allocator = another->allocator;
  self->data = another->data;
  another->data = allocator_alloc(allocator, 8);
  another->data[0] = 0;
  another->size = 1;
  another->capacity = 8;
}
static void _string_dispose(string_t self, allocator_t allocator) {
  allocator_free(allocator, &self->data);
  self->size = 0;
  self->capacity = 0;
}

type_t g_string_type = {
    .name = "cubec.core.string",
    .size = sizeof(struct _string_t),
    .init = (type_init_fn_t)_string_init,
    .dispose = (type_dispose_fn_t)_string_dispose,
    .clone = (type_clone_fn_t)_string_clone,
    .move = (type_move_fn_t)_string_move,
};
const char *string_get(string_t self) { return self->data; }
size_t string_set(string_t self, const char *str) {
  size_t size = strlen(str) + 1;
  allocator_free(self->allocator, &self->data);
  self->capacity = 8;
  while (self->capacity < size) {
    self->capacity *= 2;
  }
  self->data = allocator_alloc(self->allocator, self->capacity);
  memcpy(self->data, str, size);
  self->size = size;
  return self->size;
}

size_t  string_get_length(string_t self) { return self->size - 1; }

size_t string_concat(string_t self, const char *another) {
  size_t another_size = strlen(another);
  while (another_size + self->size - 1 >= self->capacity) {
    self->capacity *= 2;
  }
  char *data = allocator_alloc(self->allocator, self->capacity);
  memcpy(data, self->data, self->size - 1);
  allocator_free(self->allocator, &self->data);
  self->data = data;
  for (size_t idx = 0; idx < another_size; idx++) {
    self->data[self->size - 1 + idx] = another[idx];
  }
  self->data[self->size - 1 + another_size] = 0;
  self->size += another_size;
  return self->size;
}

size_t string_nconcat(string_t self, const char *another, size_t len) {
  while (len + self->size - 1 >= self->capacity) {
    self->capacity *= 2;
  }
  char *data = allocator_alloc(self->allocator, self->capacity);
  memcpy(data, self->data, self->size - 1);
  allocator_free(self->allocator, &self->data);
  self->data = data;
  for (size_t idx = 0; idx < len; idx++) {
    self->data[self->size - 1 + idx] = another[idx];
  }
  self->data[self->size - 1 + len] = 0;
  self->size += len;
  return self->size;
}