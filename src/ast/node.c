#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include <string.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>
void cubec_ast_node_initialize(cubec_allocator_t allocator,
                               cubec_ast_node_t self) {
  self->type = 0;
  self->loc.begin.offset = NULL;
  self->loc.begin.column = 0;
  self->loc.begin.line = 0;
  self->loc.end.offset = NULL;
  self->loc.end.column = 0;
  self->loc.end.line = 0;
}

void cubec_ast_node_dispose(cubec_allocator_t allocator,
                            cubec_ast_node_t self) {}

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end) {
  int32_t code = 0;
  size_t offset = 0;
  size_t len = end - position->offset;
  U8_NEXT(position->offset, offset, len, code);
  position->offset += offset;
  if (code >= 0) {
    if (code == '\\' && *position->offset == 'u') {
      position->offset++;
      position->column++;
      if (*position->offset == '{') {
        position->offset++;
        position->column++;
        code = 0;
        while (*position->offset != '}') {
          if (!*position->offset) {
            return U_INVALID_FORMAT_ERROR;
          }
          if (*position->offset >= '0' && *position->offset <= '9') {
            code *= 16 + (*position->offset - '0');
          } else if (*position->offset >= 'a' && *position->offset <= 'f') {
            code *= 16 + (*position->offset - 'a');
          } else if (*position->offset >= 'A' && *position->offset <= 'F') {
            code *= 16 + (*position->offset - 'F');
          } else {
            return U_INVALID_FORMAT_ERROR;
          }
          position->offset++;
          position->column++;
        }
        position->offset++;
        position->column++;
        return code;
      } else {
        code = 0;
        for (int32_t idx = 0; idx < 4; idx++) {
          if (!*position->offset) {
            return U_INVALID_FORMAT_ERROR;
          }
          if (*position->offset >= '0' && *position->offset <= '9') {
            code *= 16 + (*position->offset - '0');
          } else if (*position->offset >= 'a' && *position->offset <= 'f') {
            code *= 16 + (*position->offset - 'a');
          } else if (*position->offset >= 'A' && *position->offset <= 'F') {
            code *= 16 + (*position->offset - 'F');
          } else {
            return U_INVALID_FORMAT_ERROR;
          }
          position->offset++;
          position->column++;
        }
        return code;
      }
    }
    if (code == '\n' || code == 0x2028 || code == 0x2029) {
      position->line++;
      position->column = 1;
      if (*position->offset == '\r') {
        position->offset++;
      }
    } else if (code == '\r') {
      position->line++;
      position->column = 1;
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

static void cubec_error_dispose(cubec_ast_error_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->message);
}

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *message) {
  cubec_ast_error_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_error_t),
                            (cubec_dispose_fn_t)cubec_error_dispose);
  node->super.type = CUBEC_NODE_TYPE_ERROR;
  node->super.loc.begin = begin;
  node->super.loc.end = end;
  size_t len = strlen(message);
  node->message = cubec_allocator_alloc(allocator, len + 1, NULL);
  strcpy(node->message, message);
  return &node->super;
}

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position,
                                    const char *end) {
  cubec_position_t current = *position;
  while (*current.offset) {
    int32_t code = cubec_ast_read_code(&current, end);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
    if (u_isWhitespace(code)) {
      *position = current;
      continue;
    }
    if (code == '/') {
      code = cubec_ast_read_code(&current, end);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current,
                                      "Invalid unicode code");
      }
      if (code == '/') {
        while (code != '\n' && code != '\r' && code != 0x2028 &&
               code != 0x2029) {
          if (*current.offset == 0) {
            break;
          }
          code = cubec_ast_read_code(&current, end);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Invalid unicode code");
          }
        }
        *position = current;
        continue;
      }
      if (code == '*') {
        while (true) {
          if (*current.offset == 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Missing multiline comment end '*/'");
          }
          code = cubec_ast_read_code(&current, end);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Invalid unicode code");
          }
          if (code == '\\') {
            if (!*current.offset) {
              return cubec_create_ast_error(
                  allocator, *position, current,
                  "Missing multiline comment end '*/'");
            }
            code = cubec_ast_read_code(&current, end);
            if (code < 0) {
              return cubec_create_ast_error(allocator, *position, current,
                                            "Invalid unicode code");
            }
            continue;
          }
          if (code == '*' && *current.offset == '/') {
            current.offset++;
            current.column++;
            break;
          }
        }
        *position = current;
        continue;
      }
      break;
    }
    current.offset--;
    current.column--;
    break;
  }
  return NULL;
}