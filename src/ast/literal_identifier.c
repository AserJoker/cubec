#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
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
                                                   cubec_position_t *end) {
  cubec_position_t current = *position;
  uint32_t code = cubec_ast_read_code(&current, end);
  if (U_FAILURE(code)) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid unicode code");
  }
  if (!u_isIDPart(code)) {
    return NULL;
  }
  while (*current.offset) {
    code = cubec_ast_read_code(&current, end);
    if (U_FAILURE(code)) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
    if (!u_isIDPart(code)) {
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