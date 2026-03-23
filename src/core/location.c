#include "core/location.h"
#include "core/allocator.h"
#include <string.h>

char *cubec_location_get(cubec_location_t self, cubec_allocator_t allocator) {
  size_t len = self.end.offset - self.begin.offset + 1;
  char *result = cubec_allocator_alloc(allocator, len, NULL);
  result[len - 1] = 0;
  memcpy(result, self.begin.offset, len - 1);
  return result;
}
char *cubec_location_get_line(cubec_location_t self,
                              cubec_allocator_t allocator) {
  const char *begin = self.begin.offset;
  size_t col = self.begin.column - 1;
  while (col) {
    begin--;
    col--;
  }
  const char *end = begin;
  while (end != self.end.offset) {
    if (*end == '\n' || *end == '\r') {
      break;
    }
    end++;
  }
  size_t len = end - begin;
  char *s = cubec_allocator_alloc(allocator, len + 1, NULL);
  char *dst = s;
  const char *src = begin;
  while (src != end) {
    *dst++ = *src++;
  }
  *dst = 0;
  return s;
}
bool cubec_location_is(cubec_location_t self, const char *str) {
  const char *s = self.begin.offset;
  const char *ss = str;
  for (;;) {
    if (*s != *ss) {
      return false;
    }
    s++;
    ss++;
    if (*ss == 0 && s == self.end.offset) {
      break;
    }
  }
  return true;
}