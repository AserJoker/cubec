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
  for (;;) {
    if (!*current.offset) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "invalid charactor literal, missing '\''");
    }
    if (*current.offset == '\'') {
      break;
    }
    if (*current.offset == '\\') {
      current.offset += 2;
      current.column += 2;
    } else {
      current.offset++;
      current.column++;
    }
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