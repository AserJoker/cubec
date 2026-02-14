#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>

static void
cubec_ast_literal_identifier_dispose(cubec_ast_literal_identifier_t self,
                                     cubec_allocator_t allocator) {
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_literal_identifier_t
cubec_create_ast_literal_identifier(cubec_allocator_t allocator) {
  cubec_ast_literal_identifier_t identifier = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_literal_identifier_t),
      (cubec_dispose_fn_t)cubec_ast_literal_identifier_dispose);
  cubec_ast_node_initialize(allocator, &identifier->super);
  identifier->super.type = CUBEC_NODE_TYPE_LITERAL_IDENTIFIER;
  return identifier;
}

cubec_ast_node_t cubec_read_ast_literal_identifier(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end) {
  cubec_position_t current = *position;
  int32_t code = cubec_ast_read_code(&current, end);
  if (code < 0) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid unicode code");
  }
  if (!u_isIDStart(code)) {
    return NULL;
  }
  while (*current.offset) {
    cubec_position_t backup = current;
    code = cubec_ast_read_code(&current, end);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
    if (!u_isIDPart(code)) {
      current = backup;
      break;
    }
  }
  cubec_ast_literal_identifier_t identifier =
      cubec_create_ast_literal_identifier(allocator);
  identifier->super.loc.begin = *position;
  identifier->super.loc.end = current;
  *position = current;
  return &identifier->super;
}