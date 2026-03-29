#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>

cubec_ast_node_t cubec_read_ast_literal_identifier(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end,
                                                   const char *filename) {
  cubec_position_t current = *position;
  int32_t code = cubec_ast_read_code(&current, end, filename);
  if (code < 0) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid unicode code");
  }
  if (!u_isIDStart(code) && code != '_') {
    return NULL;
  }
  while (*current.offset) {
    cubec_position_t backup = current;
    code = cubec_ast_read_code(&current, end, filename);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
    if (!u_isIDPart(code)) {
      current = backup;
      break;
    }
  }
  cubec_ast_node_t identifier =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_IDENTIFIER);
  identifier->loc.begin = *position;
  identifier->loc.end = current;
  identifier->loc.filename = filename;
  *position = current;
  return identifier;
}