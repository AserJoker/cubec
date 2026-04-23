#include "core/string.h"
#include "core/allocator.h"
#include <math.h>
#include <string.h>
struct _string_t {
  char *data;
  size_t len;
  size_t capacity;
};
static void string_dispose(string_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}
string_t create_string(allocator_t allocator, string_initialize_t *initialize) {
  string_t string = allocator_alloc(allocator, sizeof(struct _string_t),
                                    (dispose_fn_t)string_dispose);
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
    string->data = allocator_alloc(allocator, string->capacity, NULL);
    if (initialize->source) {
      memcpy(string->data, initialize->source, string->len + 1);
    } else {
      string->data[0] = 0;
    }
  } else {
    string->capacity = 16;
    string->len = 0;
    string->data = allocator_alloc(allocator, string->capacity, NULL);
    string->data[0] = 0;
  }
  return string;
}

const char *string_get(string_t self) { return self->data; }

size_t string_len(string_t self) { return self->len; }

string_t string_set(string_t self, allocator_t allocator, const char *source) {
  size_t len = strlen(source);
  if (len >= self->capacity) {
    self->capacity = len + 1;
    char *str = allocator_alloc(allocator, self->capacity, NULL);
    str[0] = 0;
    allocator_free(allocator, self->data);
    self->data = str;
  }
  memcpy(self->data, source, len + 1);
  self->len = len;
  return self;
}
string_t string_concat(string_t self, allocator_t allocator,
                       const char *source) {
  size_t len = strlen(source);
  if (len + self->len + 1 >= self->capacity) {
    self->capacity = len + self->len + 1;
    char *str = allocator_alloc(allocator, self->capacity, NULL);
    memcpy(str, self->data, self->len + 1);
    allocator_free(allocator, self->data);
    self->data = str;
  }
  memcpy(&self->data[self->len], source, len + 1);
  self->len += len;
  return self;
}

string_t string_nconcat(string_t self, allocator_t allocator,
                        const char *source, size_t len) {
  if (len + self->len + 1 >= self->capacity) {
    while (len + self->len + 1 >= self->capacity) {
      self->capacity *= 2;
    }
    char *str = allocator_alloc(allocator, self->capacity, NULL);
    memcpy(str, self->data, self->len + 1);
    allocator_free(allocator, self->data);
    self->data = str;
  }
  memcpy(&self->data[self->len], source, len + 1);
  self->len += len;
  return self;
}
string_t string_concat_location(string_t self, allocator_t allocator,
                                location_t loc) {
  return string_nconcat(self, allocator, loc.begin.offset,
                        loc.end.offset - loc.begin.offset);
}
int string_compare(string_t self, const char *source) {
  return strcmp(self->data, source);
}
char *create_cstring(allocator_t allocator, const char *source) {
  size_t len = strlen(source);
  char *s = allocator_alloc(allocator, len + 1, NULL);
  strcpy(s, source);
  s[len] = 0;
  return s;
}
char *encode_cstring(allocator_t allocator, const char *source) {
  size_t len = strlen(source);
  char *s = allocator_alloc(allocator, len * 2 + 1, NULL);
  char *dst = s;
  const char *src = source;
  while (*src) {
    if (*src == '\n') {
      *dst++ = '\\';
      *dst++ = 'n';
    } else if (*src == '\r') {
      *dst++ = '\\';
      *dst++ = 'r';
    } else if (*src == '\a') {
      *dst++ = '\\';
      *dst++ = 'a';
    } else if (*src == '\b') {
      *dst++ = '\\';
      *dst++ = 'b';
    } else if (*src == '\\') {
      *dst++ = '\\';
      *dst++ = '\\';
    } else if (*src == '\t') {
      *dst++ = '\\';
      *dst++ = 't';
    } else if (*src == '\f') {
      *dst++ = '\\';
      *dst++ = 'f';
    } else if (*src == '\"') {
      *dst++ = '\\';
      *dst++ = '\"';
    } else if (*src == '\'') {
      *dst++ = '\\';
      *dst++ = '\'';
    } else {
      *dst++ = *src++;
    }
  }
  *dst = 0;
  return s;
}
const char *cstring_to_int(const char *source, size_t *value, int radix) {
  size_t val = 0;
  while (*source) {
    if (*source != '_') {
      if (radix == 2) {
        if (*source >= '0' && *source <= '1') {
          val = val * radix + (*source - '0');
        } else {
          break;
        }
      } else if (radix == 8) {
        if (*source >= '0' && *source <= '7') {
          val = val * radix + (*source - '0');
        } else {
          break;
        }
      } else if (radix == 10) {
        if (*source >= '0' && *source <= '9') {
          val = val * radix + (*source - '0');
        } else {
          break;
        }
      } else if (radix == 16) {
        if (*source >= '0' && *source <= '9') {
          val = val * radix + (*source - '0');
        } else if (*source >= 'a' && *source <= 'f') {
          val = val * radix + (*source - 'a');
        } else if (*source >= 'A' && *source <= 'F') {
          val = val * radix + (*source - 'A');
        } else {
          break;
        }
      } else {
        break;
      }
    }
    source++;
  }
  *value = val;
  return source;
}
const char *cstring_to_dec(const char *source, double *value) {
  size_t integer = 0;
  source = cstring_to_int(source, &integer, 10);
  double val = integer;
  if (*source == '.') {
    source++;
    double mask = 0.1;
    while (*source) {
      if (*source != '_') {
        if (*source >= '0' && *source <= '9') {
          val = val + (*source - '0') * mask;
          mask *= 0.1;
        } else {
          break;
        }
      }
      source++;
    }
  }
  if (*source == 'e' || *source == 'E') {
    source++;
    size_t e = 0;
    source = cstring_to_int(source, &e, 10);
    val = val * pow(10, e);
  }
  *value = val;
  return source;
}
int64_t cstring_sdb(const char *str) {
  int64_t hash = 0;
  int c;
  while ((c = *str++)) {
    hash = (hash * 65536) + c;
  }

  return hash;
}