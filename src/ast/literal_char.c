#include "ast/literal_char.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_literal_char(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end,
                                             const char *filename) {
  cubec_position_t current = *position;
  if (*current.offset != '\'') {
    return NULL;
  }
  current.offset++;
  current.column++;
  if (!*current.offset) {
    return cubec_create_ast_error(allocator, *position, current, filename,
                                  "Invalid charactor literal, missing '\''");
  }
  int32_t code = cubec_ast_read_code(&current, end, filename);
  if (code < 0) {
    return cubec_create_ast_error(allocator, *position, current, filename,
                                  "Invalid unicode code");
  }
  if (code == '\\') {
    if (!*current.offset) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid charactor literal, missing '\''");
    }
    code = cubec_ast_read_code(&current, end, filename);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid unicode code");
    }
  } else if (code == '\n' || code == '\r' || code == 0x2028 || code == 0x2029) {
    return cubec_create_ast_error(allocator, *position, current, filename,
                                  "Invalid charactor literal, missing '\''");
  }
  if (*current.offset != '\'') {
    return cubec_create_ast_error(allocator, *position, current, filename,
                                  "Invalid charactor literal, missing '\''");
  }
  current.offset++;
  current.column++;
  cubec_ast_node_t chr =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_CHAR);
  chr->loc.begin = *position;
  chr->loc.end = current;
  chr->loc.filename = filename;
  *position = current;
  return chr;
}