#include "core/location.h"
#include "core/allocator.h"
#include <stdint.h>
#include <string.h>

char *location_get(location_t self, allocator_t allocator) {
  size_t len = self.end.offset - self.begin.offset + 1;
  char *result = allocator_alloc(allocator, len, NULL);
  result[len - 1] = 0;
  memcpy(result, self.begin.offset, len - 1);
  return result;
}
static int utf32_to_utf8(uint32_t code_point, char *buffer) {
  int length = 0;
  if (code_point <= 0x7F) {
    buffer[0] = (char)(code_point & 0x7F);
    length = 1;
  } else if (code_point <= 0x7FF) {
    buffer[0] = (char)((code_point >> 6) | 0xC0);
    buffer[1] = (char)((code_point & 0x3F) | 0x80);
    length = 2;
  } else if (code_point <= 0xFFFF) {
    buffer[0] = (char)((code_point >> 12) | 0xE0);
    buffer[1] = (char)((code_point >> 6 & 0x3F) | 0x80);
    buffer[2] = (char)((code_point & 0x3F) | 0x80);
    length = 3;
  } else {
    buffer[0] = (char)((code_point >> 18) | 0xF0);
    buffer[1] = (char)((code_point >> 12 & 0x3F) | 0x80);
    buffer[2] = (char)((code_point >> 6 & 0x3F) | 0x80);
    buffer[3] = (char)((code_point & 0x3F) | 0x80);
    length = 4;
  }
  return length;
}

char *location_get_str(location_t self, allocator_t allocator) {
  size_t len = self.end.offset - self.begin.offset + 1;
  char *result = allocator_alloc(allocator, len, NULL);
  char *dst = result;
  const char *src = self.begin.offset + 1;
  while (src != self.end.offset - 2) {
    if (*src == '\\') {
      src++;
      if (*src == 'n') {
        *dst++ = '\n';
      } else if (*src == 'r') {
        *dst++ = '\r';
      } else if (*src == 'a') {
        *dst++ = '\a';
      } else if (*src == 'b') {
        *dst++ = '\b';
      } else if (*src == '\\') {
        *dst++ = '\\';
      } else if (*src == 't') {
        *dst++ = '\t';
      } else if (*src == 'f') {
        *dst++ = '\f';
      } else if (*src == 'x') {
        src++;
        char c = 0;
        for (size_t idx = 0; idx < 2; idx++) {
          c *= 16;
          if (*src >= '0' && *src <= '9') {
            c += *src - '0';
          } else if (*src >= 'a' && *src <= 'f') {
            c += *src - 'a' + 10;
          } else if (*src >= 'A' && *src <= 'F') {
            c += *src - 'A' + 10;
          }
          src++;
        }
        *dst++ = c;
      } else if (*src >= '0' && *src <= '7') {
        char c = 0;
        for (size_t idx = 0; idx < 3; idx++) {
          c *= 8;
          if (*src >= '0' && *src <= '7') {
            c += *src - '0';
          }
          src++;
        }
        *dst++ = c;
      } else if (*src == 'u') {
        src++;
        uint32_t utf32 = 0;
        if (*src == '{') {
          src++;
          for (;;) {
            if (*src >= '0' && *src <= '9') {
              utf32 *= 16;
              utf32 += *src - '0';
              src++;
            } else if (*src >= 'a' && *src <= 'f') {
              utf32 *= 16;
              utf32 += *src - 'a' + 10;
              src++;
            } else if (*src >= 'A' && *src <= 'F') {
              utf32 *= 16;
              utf32 += *src - 'A' + 10;
              src++;
            } else {
              break;
            }
          }
          src++;
        } else {
          for (size_t idx = 0; idx < 4; idx++) {
            if (*src >= '0' && *src <= '9') {
              utf32 *= 16;
              utf32 += *src - '0';
              src++;
            } else if (*src >= 'a' && *src <= 'f') {
              utf32 *= 16;
              utf32 += *src - 'a' + 10;
              src++;
            } else if (*src >= 'A' && *src <= 'F') {
              utf32 *= 16;
              utf32 += *src - 'A' + 10;
              src++;
            }
          }
        }
        dst += utf32_to_utf8(utf32, dst);
      }
    } else {
      *dst++ = *src++;
    }
  }
  *dst = 0;
  return result;
}
char *location_get_line(const location_t self, allocator_t allocator) {
  const char *begin = self.end.offset;
  size_t col = self.end.column;
  while (col >= 1) {
    begin--;
    col--;
  }
  const char *end = begin;
  while (*end) {
    if (*end == '\n' || *end == '\r') {
      break;
    }
    end++;
  }
  size_t len = end - begin;
  char *s = allocator_alloc(allocator, len + 1, NULL);
  char *dst = s;
  const char *src = begin;
  while (src != end) {
    *dst++ = *src++;
  }
  *dst = 0;
  return s;
}
bool location_is(location_t self, const char *str) {
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