#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>

ast_node_t read_ast_literal_identifier(allocator_t allocator,
                                       position_t *position, const char *end,
                                       const char *filename) {
  position_t current = *position;
  int32_t code = ast_read_code(&current, end, filename);
  if (code < 0) {
    return create_ast_error(allocator, *position, current, filename,
                            "invalid unicode code");
  }
  if (!u_isIDStart(code) && code != '_') {
    return NULL;
  }
  while (*current.offset) {
    position_t backup = current;
    code = ast_read_code(&current, end, filename);
    if (code < 0) {
      return create_ast_error(allocator, *position, current, filename,
                              "invalid unicode code");
    }
    if (!u_isIDPart(code)) {
      current = backup;
      break;
    }
  }
  ast_node_t identifier =
      create_ast_node(allocator, NODE_TYPE_LITERAL_IDENTIFIER);
  identifier->loc.begin = *position;
  identifier->loc.end = current;
  identifier->loc.filename = filename;
  *position = current;
  return identifier;
}