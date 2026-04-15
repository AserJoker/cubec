#include "ast/literal_char.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/position.h"

ast_node_t read_ast_literal_char(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename) {
  if (*position->offset != '\'') {
    return NULL;
  }
  position_t current = *position;
  int32_t code = *current.offset;
  current.offset++;
  current.column++;
  if (code != '\'') {
    return NULL;
  }
  for (;;) {
    if (!*current.offset) {
      return create_ast_error(allocator, *position, current, filename,
                              "invalid string literal, missing '\''");
    }
    position_t escape_start = current;
    code = *current.offset;
    current.offset++;
    current.column++;
    if (code == '\n' || code == '\r' || code == 0x2028 || code == 2029) {
      return create_ast_error(allocator, *position, current, filename,
                              "invalid string literal, missing '\''");
    }
    if (code == '\\') {
      if (!*current.offset) {
        return create_ast_error(allocator, escape_start, current, filename,
                                "invalid string literal, missing '\''");
      }
      code = *current.offset;
      current.offset++;
      current.column++;
      if (code == 'n' || code == 'r' || code == 'a' || code == 'b' ||
          code == '\\' || code == 't' || code == 'f') {
        continue;
      }
      if (code == 'x') {
        for (size_t idx = 0; idx < 2; idx++) {
          position_t backup = current;
          code = *current.offset;
          current.offset++;
          current.column++;
          if (code >= '0' && code <= '9' || code >= 'a' && code <= 'f' ||
              code >= 'A' && code <= 'F') {
            ;
          } else if (idx == 0) {
            current = backup;
            return create_ast_error(allocator, escape_start, current, filename,
                                    "invalid unicode code");
          } else {
            current = backup;
            break;
          }
        }
        continue;
      }
      if (code >= '0' && code <= '7') {
        size_t c = code - '0';
        for (size_t idx = 0; idx < 2; idx++) {
          c *= 8;
          position_t backup = current;
          code = *current.offset;
          current.offset++;
          current.column++;
          if (code >= '0' && code <= '7') {
            c += code - '0';
          } else {
            c /= 8;
            current = backup;
            break;
          }
        }
        if (c > UINT8_MAX) {
          return create_ast_error(allocator, escape_start, current, filename,
                                  "invalid escape code");
        }
        continue;
      }
      if (code == 'u') {
        uint32_t utf32 = 0;
        if (*current.offset == '{') {
          current.offset++;
          current.column++;
          if (!((*current.offset >= '0' && *current.offset <= '9') ||
                (*current.offset >= 'a' && *current.offset <= 'f' ||
                 *current.offset >= 'A' && *current.offset <= 'F'))) {
            return create_ast_error(allocator, escape_start, current, filename,
                                    "invalid escape code");
          }
          while (true) {
            if (*current.offset >= '0' && *current.offset <= '9') {
              utf32 *= 16;
              utf32 += *current.offset - '0';
              current.offset++;
              current.column++;
            } else if (*current.offset >= 'a' && *current.offset <= 'f') {
              utf32 *= 16;
              utf32 += *current.offset - 'a' + 10;
              current.offset++;
              current.column++;
            } else if (*current.offset >= 'A' && *current.offset <= 'F') {
              utf32 *= 16;
              utf32 += *current.offset - 'A' + 10;
              current.offset++;
              current.column++;
            } else if (*current.offset == '}') {
              current.offset++;
              current.column++;
              break;
            } else {
              return create_ast_error(allocator, escape_start, current,
                                      filename, "invalid escape code");
            }
            if (utf32 > 0x10FFFF || (utf32 >= 0xD800 && utf32 <= 0xDFFF)) {
              return create_ast_error(allocator, escape_start, current,
                                      filename, "invalid unicode code");
            }
          }
        } else {
          for (size_t idx = 0; idx < 4; idx++) {
            if (*current.offset >= '0' && *current.offset <= '9') {
              utf32 *= 16;
              utf32 += *current.offset - '0';
              current.offset++;
              current.column++;
            } else if (*current.offset >= 'a' && *current.offset <= 'f') {
              utf32 *= 16;
              utf32 += *current.offset - 'a' + 10;
              current.offset++;
              current.column++;
            } else if (*current.offset >= 'A' && *current.offset <= 'F') {
              utf32 *= 16;
              utf32 += *current.offset - 'A' + 10;
              current.offset++;
              current.column++;
            } else {
              return create_ast_error(allocator, escape_start, current,
                                      filename, "invalid unicode code");
            }
            if (utf32 > 0x10FFFF || (utf32 >= 0xD800 && utf32 <= 0xDFFF)) {
              return create_ast_error(allocator, escape_start, current,
                                      filename, "invalid unicode code");
            }
          }
        }
        continue;
      }
      return create_ast_error(allocator, escape_start, current, filename,
                              "invalid escape code");
    }
    if (code == '\'') {
      break;
    }
  }
  ast_node_t chr = create_ast_node(allocator, NODE_TYPE_LITERAL_CHAR);
  chr->loc.begin = *position;
  chr->loc.end = current;
  chr->loc.filename = filename;
  *position = current;
  return chr;
}