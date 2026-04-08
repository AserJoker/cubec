#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_literal_string(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end,
                                               const char *filename) {
  cubec_position_t current = *position;
  int32_t code = cubec_ast_read_code(&current, end, filename);
  if (code < 0) {
    return cubec_create_ast_error(allocator, *position, current, filename,
                                  "Invalid unicode code");
  }
  if (code != '\"') {
    return NULL;
  }
  for (;;) {
    if (!*current.offset) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid string literal, missing '\"'");
    }
    code = cubec_ast_read_code(&current, end, filename);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid unicode code");
    }
    if (code == '\n' || code == '\r' || code == 0x2028 || code == 2029) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid string literal, missing '\"'");
    }
    if (code == '\\') {
      if (!*current.offset) {
        return cubec_create_ast_error(allocator, *position, current, filename,
                                      "Invalid string literal, missing '\"'");
      }
      code = cubec_ast_read_code(&current, end, filename);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current, filename,
                                      "Invalid unicode code");
      }
      continue;
    }
    if (code == '\"') {
      break;
    }
  }
  cubec_ast_node_t string =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_STRING);
  string->loc.begin = *position;
  string->loc.end = current;
  string->loc.filename = filename;
  *position = current;
  return string;
}