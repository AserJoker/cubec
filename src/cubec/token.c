#include "cubec/token.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/location.h"
#include "core/position.h"
#include "core/token.h"
#include "core/vec.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>

static token_t create_eof_token(allocator_t allocator, location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_EOF, location});
}

static token_t create_identifier_token(allocator_t allocator,
                                       location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_IDENTIFIER, location});
}
static token_t create_keyword_token(allocator_t allocator,
                                    location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_KEYWORD, location});
}
static token_t create_numeric_token(allocator_t allocator,
                                    location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_NUMERIC, location});
}
static token_t create_string_token(allocator_t allocator, location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_STRING, location});
}

static token_t create_char_token(allocator_t allocator, location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_CHAR, location});
}

static token_t create_symbol_token(allocator_t allocator, location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_SYMBOL, location});
}
static token_t create_whitespace_token(allocator_t allocator,
                                       location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_WHITESPACE, location});
}
static token_t create_comment_token(allocator_t allocator,
                                    location_t location) {
  return allocator_create(allocator, &g_token_type,
                          &(token_init_t){CUBEC_TOKEN_COMMENT, location});
}
static token_t create_multiline_comment_token(allocator_t allocator,
                                              location_t location) {
  return allocator_create(
      allocator, &g_token_type,
      &(token_init_t){CUBEC_TOKEN_MULTILINE_COMMENT, location});
}
/**
 * @brief 从 UTF-8 字符串中读取一个 Unicode 码点
 *
 * @param source 指向 UTF-8 字符串的指针
 * @param length 输出参数，返回该码点所占的字节数
 * @return uint32_t 解码后的 Unicode 码点值。如果出错，返回 0xFFFD
 */
static uint32_t read_unicode(const char *source, size_t *length) {
  if (source == NULL || length == NULL) {
    *length = 1;
    return 0xFFFD;
  }

  unsigned char c = (unsigned char)*source;
  uint32_t codepoint = 0;
  size_t bytes_needed = 0;

  // 1. 确定首字节对应的预期字节数
  if ((c & 0x80) == 0) {
    // 0xxxxxxx: 1 字节 ASCII
    bytes_needed = 1;
  } else if ((c & 0xE0) == 0xC0) {
    // 110xxxxx: 2 字节
    bytes_needed = 2;
  } else if ((c & 0xF0) == 0xE0) {
    // 1110xxxx: 3 字节
    bytes_needed = 3;
  } else if ((c & 0xF8) == 0xF0) {
    // 11110xxx: 4 字节
    bytes_needed = 4;
  } else {
    // 非法起始字节 (如 10xxxxxx 作为开头，或 11111xxx 等保留位)
    *length = 1;
    return 0xFFFD;
  }

  // 2. 解码第一个字节的有效载荷
  switch (bytes_needed) {
  case 1:
    codepoint = c;
    break;
  case 2:
    codepoint = c & 0x1F; // 取低 5 位
    break;
  case 3:
    codepoint = c & 0x0F; // 取低 4 位
    break;
  case 4:
    codepoint = c & 0x07; // 取低 3 位
    break;
  default:
    *length = 1;
    return 0xFFFD;
  }

  // 3. 解码后续字节并验证格式
  for (size_t i = 1; i < bytes_needed; ++i) {
    unsigned char next_byte = (unsigned char)*(source + i);

    // 检查后续字节是否为 continuation byte (10xxxxxx)
    if ((next_byte & 0xC0) != 0x80) {
      // 不是合法的后续字节，视为错误
      *length = 1;
      return 0xFFFD;
    }

    codepoint = (codepoint << 6) | (next_byte & 0x3F);
  }

  // 4. 可选：验证是否产生过长的编码 (Overlong encoding)
  // 例如：U+0000 应该用 1 字节表示，而不是 2/3/4 字节
  // 这里为了简化，通常标准解析器会在此处增加范围检查，
  // 但如果仅做基本解析，上述步骤已能处理大多数情况。
  // 若需严格合规，可在此添加对 codepoint 范围的检查。

  *length = bytes_needed;
  return codepoint;
}

static const char *symbols[] = {
    "&&=", "||=", "...", "==", "!=", ">>", "<<", "+=", "-=", "*=", "/=", "%=",
    "&=",  "|=",  "^=", "~=", "&&", "||", ">=", "<=", "=",  "!",  "+",
    "-",   "*",   "/",  "&",  "|",  "^",  "?",  ",",  ".",  "<",  ">",
    ";",   ":",   "%",  "[",  "]",  "{",  "}",  "(",  ")",  "~",  0,
};

static token_t read_symbol_token(allocator_t allocator, position_t *position,
                                 const char *filename) {
  position_t current = *position;
  size_t idx = 0;
  for (idx = 0; symbols[idx]; idx++) {
    size_t i = 0;
    for (i = 0; symbols[idx][i]; i++) {
      if (current.offset[i] != symbols[idx][i]) {
        break;
      }
    }
    if (!symbols[idx][i]) {
      current.offset += i;
      current.column += i;
      break;
    }
  }
  if (!symbols[idx]) {
    return NULL;
  }
  token_t token = create_symbol_token(
      allocator, (location_t){filename, *position, current});
  *position = current;
  return token;
}
static token_t read_whitespace_token(allocator_t allocator,
                                     position_t *position,
                                     const char *filename) {
  position_t current = *position;
  size_t length = 0;
  uint32_t code = read_unicode(current.offset, &length);
  if (code == '\n' || code == '\r' || code == 0x2028 || code == 2029) {
    current.offset += length;
    current.column = 0;
    current.line++;
    if (code == '\r' && *current.offset == '\n') {
      current.offset++;
    }
  } else if (u_isWhitespace(code)) {
    current.offset += length;
    current.column += length;
  } else {
    return NULL;
  }
  token_t token = create_whitespace_token(
      allocator, (location_t){filename, *position, current});
  *position = current;
  return token;
}
static token_t read_comment_token(allocator_t allocator, position_t *position,
                                  const char *filename) {
  position_t current = *position;
  if (*current.offset == '/' && *(current.offset + 1) == '/') {
    current.offset += 2;
    current.column += 2;
    size_t length = 0;
    uint32_t code = 0;
    while (true) {
      code = read_unicode(current.offset, &length);
      if (code == 0 || code == '\n' || code == '\r' || code == 0x2028 ||
          code == 2029) {
        break;
      }
      current.offset += length;
      current.column += length;
    }
    token_t token = create_comment_token(
        allocator, (location_t){filename, *position, current});
    *position = current;
    return token;
  } else {
    return NULL;
  }
}

static token_t read_multiline_comment_token(allocator_t allocator,
                                            position_t *position,
                                            const char *filename) {
  position_t current = *position;
  if (*current.offset == '/' && *(current.offset + 1) == '*') {
    current.offset += 2;
    current.column += 2;
    size_t length = 0;
    while (true) {
      if (!*current.offset) {
        THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
              filename, current.line + 1, current.column);
      }
      if (*current.offset == '*' && *(current.offset + 1) == '/') {
        current.offset += 2;
        current.column += 2;
        break;
      }
      uint32_t code = read_unicode(current.offset, &length);
      current.offset += length;
      current.column += length;
      if (code == '\\') {
        code = read_unicode(current.offset, &length);
        current.offset += length;
        current.column += length;
      } else if (code == '\n' || code == 0x2028 || code == 0x2029) {
        current.column = 0;
        current.line++;
      } else if (code == '\r') {
        current.column = 0;
        current.line++;
        if (*current.offset == '\n') {
          current.offset++;
        }
      }
    }
    token_t token = create_multiline_comment_token(
        allocator, (location_t){filename, *position, current});
    *position = current;
    return token;
  } else {
    return NULL;
  }
}

static token_t read_numeric_token(allocator_t allocator, position_t *position,
                                  const char *filename) {
  position_t current = *position;
  if (*current.offset >= '0' && *current.offset <= '9') {
    current.offset++;
    current.column++;
    if (*current.offset == 'x' || *current.offset == 'X') {
      current.offset++;
      current.column++;
      while (true) {
        if ((*current.offset >= 'a' && *current.offset <= 'f') ||
            (*current.offset >= 'A' && *current.offset <= 'F') ||
            (*current.offset >= '0' && *current.offset <= '9')) {
          current.offset++;
          current.column++;
        } else {
          break;
        }
      }
    } else if (*current.offset == 'o' || *current.offset == 'O') {
      current.offset++;
      current.column++;
      while (true) {
        if (*current.offset >= '0' && *current.offset <= '7') {
          current.offset++;
          current.column++;
        } else {
          break;
        }
      }
    } else if (*current.offset == 'b' || *current.offset == 'B') {
      current.offset++;
      current.column++;
      while (true) {
        if (*current.offset >= '0' && *current.offset <= '1') {
          current.offset++;
          current.column++;
        } else {
          break;
        }
      }
    } else {
      while (true) {
        if (*current.offset >= '0' && *current.offset <= '9') {
          current.offset++;
          current.column++;
        } else {
          break;
        }
      }
      if (*current.offset == '.') {
        current.offset++;
        current.column++;
        if (*current.offset >= '0' && *current.offset <= '9') {
          while (true) {
            if (*current.offset >= '0' && *current.offset <= '9') {
              current.offset++;
              current.column++;
            } else {
              break;
            }
          }
        } else {
          THROW(NULL,
                "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                filename, current.line + 1, current.column);
        }
      }
      if (*current.offset == 'e' || *current.offset == 'E') {
        current.offset++;
        current.column++;
        if (*current.offset >= '0' && *current.offset <= '9') {
          while (true) {
            if (*current.offset >= '0' && *current.offset <= '9') {
              current.offset++;
              current.column++;
            } else {
              break;
            }
          }
        } else {
          THROW(NULL,
                "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                filename, current.line + 1, current.column);
        }
      }
    }
  } else {
    return NULL;
  }
  token_t token = create_numeric_token(
      allocator, (location_t){filename, *position, current});
  *position = current;
  return token;
}
static token_t read_string_token(allocator_t allocator, position_t *position,
                                 const char *filename) {
  position_t current = *position;
  if (*current.offset != '\"') {
    return NULL;
  }
  current.offset++;
  current.column++;
  while (true) {
    if (!*current.offset) {
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
            filename, current.line + 1, current.column);
    }
    size_t length = 0;
    uint32_t code = read_unicode(current.offset, &length);
    if (code == '\n' || code == '\r' || code == 0x2028 || code == 2029) {
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
            filename, current.line + 1, current.column);
    }
    if (*current.offset == '\"') {
      current.offset++;
      current.column++;
      break;
    }
    if (*current.offset == '\\') {
      current.offset++;
      current.column++;
      if (*current.offset == 'x') {
        current.offset++;
        current.column++;
        for (int i = 0; i < 2; i++) {
          if ((*current.offset >= '0' && *current.offset <= '9') ||
              (*current.offset >= 'a' && *current.offset <= 'f') ||
              (*current.offset >= 'A' && *current.offset <= 'F')) {
            current.offset++;
            current.column++;
          } else {
            break;
          }
        }
      } else if (*current.offset == 'u') {
        current.offset++;
        current.column++;
        if (*current.offset != '{') {
          THROW(NULL,
                "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                filename, current.line + 1, current.column);
        }
        current.offset++;
        current.column++;
        while (*current.offset != '}') {
          if ((*current.offset >= '0' && *current.offset <= '9') ||
              (*current.offset >= 'a' && *current.offset <= 'f') ||
              (*current.offset >= 'A' && *current.offset <= 'F')) {
            current.offset++;
            current.column++;
          } else {
            THROW(NULL,
                  "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                  filename, current.line + 1, current.column);
          }
        }
        current.offset++;
        current.column++;
      } else if (*current.offset == 'n' || *current.offset == 't' ||
                 *current.offset == 'r' || *current.offset == '\\' ||
                 *current.offset == '\'' || *current.offset == '"' ||
                 *current.offset == '0') {
        current.offset++;
        current.column++;
      } else {
        THROW(NULL,
              "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
              filename, current.line + 1, current.column);
      }
    } else {
      current.column++;
      current.offset++;
    }
  }
  token_t token = create_string_token(
      allocator, (location_t){filename, *position, current});
  *position = current;
  return token;
}
static token_t read_char_token(allocator_t allocator, position_t *position,
                               const char *filename) {
  position_t current = *position;
  if (*current.offset != '\'') {
    return NULL;
  }
  current.offset++;
  current.column++;
  if (!*current.offset) {
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
          filename, current.line + 1, current.column);
  }
  unsigned char code = 0;
  if (*current.offset == '\\') {
    current.offset++;
    current.column++;
    if (!*current.offset) {
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
            filename, current.line + 1, current.column);
    }
    switch (*current.offset) {
    case 'n':
      code = '\n';
      current.offset++;
      current.column++;
      break;
    case 't':
      code = '\t';
      current.offset++;
      current.column++;
      break;
    case 'r':
      code = '\r';
      current.offset++;
      current.column++;
      break;
    case '\\':
      code = '\\';
      current.offset++;
      current.column++;
      break;
    case '\'':
      code = '\'';
      current.offset++;
      current.column++;
      break;
    case '0':
      code = '\0';
      current.offset++;
      current.column++;
      break;
    case 'x':
      current.offset++;
      current.column++;
      for (int i = 0; i < 2; i++) {
        if ((*current.offset >= '0' && *current.offset <= '9') ||
            (*current.offset >= 'a' && *current.offset <= 'f') ||
            (*current.offset >= 'A' && *current.offset <= 'F')) {
          current.offset++;
          current.column++;
        } else {
          break;
        }
      }
      break;
    case 'u':
      current.offset++;
      current.column++;
      if (*current.offset != '{') {
        THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
              filename, current.line + 1, current.column);
      }
      current.offset++;
      current.column++;
      while (*current.offset != '}') {
        if ((*current.offset >= '0' && *current.offset <= '9') ||
            (*current.offset >= 'a' && *current.offset <= 'f') ||
            (*current.offset >= 'A' && *current.offset <= 'F')) {
          current.offset++;
          current.column++;
        } else {
          THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                filename, current.line + 1, current.column);
        }
      }
      current.offset++;
      current.column++;
      break;
    default:
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
            filename, current.line + 1, current.column);
    }
  } else {
    code = (unsigned char)*current.offset;
    if (code == '\0' || code == '\'' || code == '"' || code == '\\' ||
        code == '\n' || code == '\r') {
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
            filename, current.line + 1, current.column);
    }
    current.offset++;
    current.column++;
  }
  if (*current.offset != '\'') {
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
          filename, current.line + 1, current.column);
  }
  current.offset++;
  current.column++;
  token_t token = create_char_token(
      allocator, (location_t){filename, *position, current});
  *position = current;
  return token;
}
static const char *keywords[] = {
    "break",  "case",    "comptime", "const", "continue", "defer",
    "defer",  "do",      "else",     "enum",  "export",   "extern",
    "for",    "foreach", "func",     "if",    "import",   "in",
    "inline", "mutable", "of",       "pub",   "register", "return",
    "struct", "switch",  "test",     "union", "volatile", "while",
    0,
};
static token_t read_identifier_token(allocator_t allocator,
                                     position_t *position,
                                     const char *filename) {
  position_t current = *position;
  size_t length = 0;
  uint32_t code = read_unicode(current.offset, &length);
  if (u_isIDStart(code) || code == '_') {
    current.offset += length;
    current.column += length;
    while (true) {
      if (!*current.offset) {
        break;
      }
      code = read_unicode(current.offset, &length);
      if (!u_isIDPart(code) && code != '_') {
        break;
      }
      current.offset += length;
      current.column += length;
    }
    location_t location = (location_t){filename, *position, current};
    token_t token = NULL;
    for (size_t idx = 0; keywords[idx]; idx++) {
      if (location_is(&location, keywords[idx])) {
        token = create_keyword_token(allocator, location);
        break;
      }
    }
    if (!token) {
      token = create_identifier_token(allocator, location);
    }
    *position = current;
    return token;
  }
  return NULL;
}
static token_t read_eof_token(allocator_t allocator, position_t *position,
                              const char *filename) {
  if (*position->offset) {
    return NULL;
  }
  return create_eof_token(allocator,
                          (location_t){filename, *position, *position});
}

token_t read_token(allocator_t allocator, position_t *position,
                   const char *filename) {
  token_t token = NULL;
  if (!token) {
    token = TRY(NULL, read_eof_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_numeric_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_string_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_char_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_comment_token(allocator, position, filename));
  }
  if (!token) {
    token =
        TRY(NULL, read_multiline_comment_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_whitespace_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_identifier_token(allocator, position, filename));
  }
  if (!token) {
    token = TRY(NULL, read_symbol_token(allocator, position, filename));
  }
  if (!token) {
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
          filename, position->line + 1, position->column);
  }
  return token;
}
vec_t resolve_token_list(allocator_t allocator, const char *filename,
                         const char *source) {
  vec_t vec = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  position_t position = {
      .column = 0,
      .line = 0,
      .offset = source,
  };
  while (true) {
    token_t token =
        TRY_LOCAL(onerror, read_token(allocator, &position, filename));
    vec_push(vec, token);
    if (token_get_kind(token) == CUBEC_TOKEN_EOF) {
      break;
    }
  }
  return vec;
onerror:
  allocator_free(allocator, &vec);
  return NULL;
}