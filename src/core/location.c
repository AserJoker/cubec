#include "core/location.h"
#include "core/allocator.h"
#include <string.h>

char *location_get(location_t *loc, allocator_t allocator) {
  size_t len = loc->end.offset - loc->begin.offset + 1;
  char *str = allocator_alloc(allocator, len);
  strncpy(str, loc->begin.offset, len);
  str[len] = 0;
  return str;
}
bool location_is(location_t *loc, const char *str) {
  size_t length = loc->end.offset - loc->begin.offset;
  return strncmp(loc->begin.offset, str, length) == 0;
}