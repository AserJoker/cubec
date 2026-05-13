#include "reader/token.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/position.h"
#include "reader/token_type.h"
#include <string.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>
#include <unicode/urename.h>
static int32_t read_code(position_t *position, const char *end) {
  int32_t code = 0;
  size_t offset = 0;
  size_t len = end - position->offset;
  U8_NEXT(position->offset, offset, len, code);
  position->offset += offset;
  if (code >= 0) {
    if (code == '\n' || code == 0x2028 || code == 0x2029) {
      position->line++;
      position->column = 0;
      if (*position->offset == '\r') {
        position->offset++;
      }
    } else if (code == '\r') {
      position->line++;
      position->column = 0;
      if (*position->offset == '\n') {
        position->offset++;
      }
    } else {
      position->column += offset;
    }
  } else {
    position->column += offset;
  }
  return code;
}

static token_t create_token(allocator_t allocator, token_type_t type,
                            location_t loc) {
  token_t self = allocator_alloc(allocator, sizeof(struct _token_t), NULL);
  self->type = type;
  self->loc = loc;
  return self;
}

token_t read_space_token(allocator_t allocator, position_t *position,
                         const char *end, const char *filename) {
  position_t current = *position;
  int32_t code = read_code(&current, end);
  if (code < 0) {
    *position = current;
    return NULL;
  }
  if (!u_isWhitespace(code)) {
    return NULL;
  }
  for (;;) {
    position_t curr = current;
    code = read_code(&curr, end);
    if (code < 0) {
      *position = current;
      return NULL;
    }
    if (!u_isWhitespace(code)) {
      break;
    } else {
      current = curr;
    }
  }
  token_t token = create_token(allocator, TOKEN_TYPE_SPACE,
                               (location_t){*position, current, filename});
  *position = current;
  return token;
}
token_t read_comment_token(allocator_t allocator, position_t *position,
                           const char *end, const char *filename) {
  position_t current = *position;
  if (*current.offset != '/') {
    return NULL;
  }
  current.offset++;
  current.column++;
  if (*current.offset == '/') {
    current.offset++;
    current.column++;
    for (;;) {
      if (*current.offset == '\\') {
        current.offset++;
        current.column++;
        int32_t code = read_code(&current, end);
        if (code < 0) {
          *position = current;
          return NULL;
        }
      } else if (current.offset == end) {
        break;
      } else {
        int32_t code = read_code(&current, end);
        if (code < 0) {
          *position = current;
          return NULL;
        }
        if (code == '\n' || code == '\r') {
          break;
        }
      }
    }
    token_t token = create_token(allocator, TOKEN_TYPE_COMMENT,
                                 (location_t){*position, current, filename});
    *position = current;
    return token;
  } else if (*current.offset == '*') {
    current.offset++;
    current.column++;
    for (;;) {
      if (*current.offset == '\\') {
        current.offset++;
        current.column++;
        int32_t code = read_code(&current, end);
        if (code < 0) {
          *position = current;
          return NULL;
        }
      } else if (current.offset == end) {
        *position = current;
        return NULL;
      } else {
        int32_t code = read_code(&current, end);
        if (code < 0) {
          *position = current;
          return NULL;
        }
        if (code == '*' && *current.offset == '/') {
          current.offset++;
          current.column++;
          break;
        }
      }
    }
    token_t token = create_token(allocator, TOKEN_TYPE_COMMENT,
                                 (location_t){*position, current, filename});
    *position = current;
    return token;
  } else {
    *position = current;
    return NULL;
  }
}

token_t read_numeric_token(allocator_t allocator, position_t *position,
                           const char *end, const char *filename) {
  position_t current = *position;

  if (*current.offset == '0' &&
      (*(current.offset + 1) == 'x' || *(current.offset + 1) == 'X')) {
    current.offset += 2;
    current.column += 2;
    if ((*current.offset >= '0' && *current.offset <= '9') ||
        (*current.offset >= 'A' && *current.offset <= 'F') ||
        (*current.offset >= 'a' && *current.offset <= 'f')) {
      while ((*current.offset >= '0' && *current.offset <= '9') ||
             (*current.offset >= 'A' && *current.offset <= 'F') ||
             (*current.offset >= 'a' && *current.offset <= 'f')) {
        current.offset++;
        current.column++;
      }
      token_t token = create_token(allocator, TOKEN_TYPE_NUMERIC,
                                   (location_t){*position, current, filename});
      *position = current;
      return token;
    } else {
      *position = current;
      return NULL;
    }
  }
  if (*current.offset == '0' &&
      (*(current.offset + 1) == 'o' || *(current.offset + 1) == 'O')) {
    current.offset += 2;
    current.column += 2;
    if ((*current.offset >= '0' && *current.offset <= '7')) {
      while ((*current.offset >= '0' && *current.offset <= '7')) {
        current.offset++;
        current.column++;
      }
      token_t token = create_token(allocator, TOKEN_TYPE_NUMERIC,
                                   (location_t){*position, current, filename});
      *position = current;
      return token;
    } else {
      *position = current;
      return NULL;
    }
  }
  if (*current.offset == '0' &&
      (*(current.offset + 1) == 'b' || *(current.offset + 1) == 'B')) {
    current.offset += 2;
    current.column += 2;
    if ((*current.offset >= '0' && *current.offset <= '1')) {
      while ((*current.offset >= '0' && *current.offset <= '1')) {
        current.offset++;
        current.column++;
      }
      token_t token = create_token(allocator, TOKEN_TYPE_NUMERIC,
                                   (location_t){*position, current, filename});
      *position = current;
      return token;
    } else {
      *position = current;
      return NULL;
    }
  }
  if (*current.offset == '.' &&
      (*(current.offset + 1) >= '0' && *(current.offset + 1) <= '9')) {
    current.offset++;
    current.column++;
    while (*current.offset >= '0' && *current.offset <= '9') {
      current.column++;
      current.offset++;
    }
  } else if (*current.offset >= '0' && *current.offset < '9') {
    while (*current.offset >= '0' && *current.offset <= '9') {
      current.column++;
      current.offset++;
    }
    if (*current.offset == 'e' || *current.offset == 'E') {
      current.column++;
      current.offset++;
      while (*current.offset >= '0' && *current.offset <= '9') {
        current.column++;
        current.offset++;
      }
    }
  } else {
    return NULL;
  }
  if (*current.offset == 'e' || *current.offset == 'E') {
    current.offset++;
    current.column++;
    while (*current.offset >= '0' && *current.offset <= '9') {
      current.column++;
      current.offset++;
    }
  }
  token_t token = create_token(allocator, TOKEN_TYPE_NUMERIC,
                               (location_t){*position, current, filename});
  *position = current;
  return token;
}

token_t read_string_token(allocator_t allocator, position_t *position,
                          const char *end, const char *filename) {
  if (*position->offset != '\"') {
    return NULL;
  }
  position_t current = *position;
  current.column++;
  current.offset++;
  for (;;) {
    if (current.offset == end) {
      *position = current;
      return NULL;
    } else if (*current.offset == '\\') {
      current.offset++;
      current.column++;
      int32_t code = read_code(&current, end);
      if (code < 0) {
        *position = current;
        return NULL;
      }
    } else if (*current.offset == '\"') {
      current.offset++;
      current.column++;
      break;
    }
  }
  token_t token = create_token(allocator, TOKEN_TYPE_STRING,
                               (location_t){*position, current, filename});
  *position = current;
  return token;
}

token_t read_charator_token(allocator_t allocator, position_t *position,
                            const char *end, const char *filename) {
  if (*position->offset != '\'') {
    return NULL;
  }
  position_t current = *position;
  current.column++;
  current.offset++;
  for (;;) {
    if (current.offset == end) {
      *position = current;
      return NULL;
    } else if (*current.offset == '\\') {
      current.offset++;
      current.column++;
      int32_t code = read_code(&current, end);
      if (code < 0) {
        *position = current;
        return NULL;
      }
    } else if (*current.offset == '\'') {
      current.offset++;
      current.column++;
      break;
    }
  }
  token_t token = create_token(allocator, TOKEN_TYPE_STRING,
                               (location_t){*position, current, filename});
  *position = current;
  return token;
}

static const char *symbols[] = {
    ">>=", "<<=", ">>", "<<", "&&", "||", "??", "==", "!=", "+=", "-=", "*=",
    "/=",  "%=",  "^=", "&=", "|=", "~=", ">=", "<=", "->", "[[", "]]", "=",
    "+",   "-",   "*",  "/",  "%",  "&",  "|",  "~",  "^",  "!",  ":",  ";",
    ",",   "(",   ")",  "{",  "}",  "[",  "]",  "<",  ">",  ".",  0};
token_t read_symbol_token(allocator_t allocator, position_t *position,
                          const char *end, const char *filename) {
  for (size_t idx = 0; symbols[idx]; idx++) {
    position_t current = *position;
    const char *symbol = symbols[idx];
    for (;;) {
      if (!*symbol) {
        token_t token =
            create_token(allocator, TOKEN_TYPE_SYMBOL,
                         (location_t){*position, current, filename});
        *position = current;
        return token;
      }
      if (*symbol != *current.offset) {
        break;
      }
      current.offset++;
      current.column++;
      symbol++;
    }
  }
  return NULL;
}
static const char *keywords[] = {
    "func",   "return",   "struct",  "enum",    "union",    "if",
    "else",   "while",    "for",     "foreach", "break",    "continue",
    "switch", "case",     "default", "do",      "comptime", "extern",
    "inline", "register", "const",   "let",     "volatile", 0};
token_t read_identifier_token(allocator_t allocator, position_t *position,
                              const char *end, const char *filename) {
  position_t current = *position;
  int32_t code = read_code(&current, end);
  if (code < 0) {
    *position = current;
    return NULL;
  }
  if (u_isIDStart(code) || code == '_') {
    for (;;) {
      position_t curr = current;
      code = read_code(&curr, end);
      if (code < 0) {
        *position = current;
        return NULL;
      }
      if (u_isIDPart(code) || code == '_') {
        current = curr;
      } else {
        token_t token =
            create_token(allocator, TOKEN_TYPE_IDENTIFIER,
                         (location_t){*position, current, filename});
        *position = current;
        for (size_t idx = 0; keywords[idx]; idx++) {
          const char *keyword = keywords[idx];
          if (location_is(token->loc, keyword)) {
            token->type = TOKEN_TYPE_KEYWORD;
            return token;
          }
        }
        return token;
      }
    }
  }
  return NULL;
}

token_t read_token(allocator_t allocator, position_t *position, const char *end,
                   const char *filename) {
  token_t token = read_space_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_comment_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_numeric_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_string_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_charator_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_identifier_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  token = read_symbol_token(allocator, position, end, filename);
  if (token) {
    return token;
  }
  return NULL;
}
array_t read_token_list(allocator_t allocator, position_t *position,
                        const char *end, const char *filename) {
  array_initialize_t init = {
      .autofree = true,
  };
  array_t list = create_array(allocator, &init);
  for (;;) {
    token_t token = read_token(allocator, position, end, filename);
    if (!token) {
      break;
    }
    array_push(list, token);
  }
  if (position->offset != end) {
    allocator_free(allocator, list);
    return NULL;
  }
  return list;
}

static void token_stream_dispose(token_stream_t self, allocator_t allocator) {
  allocator_free(allocator, self->tokens);
}

token_stream_t create_token_stream(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename) {
  token_stream_t stream =
      allocator_alloc(allocator, sizeof(struct _token_stream_t),
                      (dispose_fn_t)token_stream_dispose);
  stream->tokens = read_token_list(allocator, position, end, filename);
  if (!stream->tokens) {
    allocator_free(allocator, stream);
    return NULL;
  }
  stream->position = 0;
  return stream;
}
token_t stream_eat(token_stream_t stream) {
  if (stream->position < array_get_size(stream->tokens)) {
    return array_get(stream->tokens, stream->position++);
  }
  return NULL;
}