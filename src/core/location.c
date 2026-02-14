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
bool cubec_location_is(cubec_location_t self, const char *str) {
  const char *s = self.begin.offset;
  while (*str) {
    if (*s != *str) {
      return false;
    }
    s++;
    str++;
  }
  return true;
}