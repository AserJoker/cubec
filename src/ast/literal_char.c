#include "ast/literal_char.h"
#include "ast/node_type.h"
#include "core/position.h"
static void cubec_ast_literal_char_dispose(cubec_ast_literal_char_t self,
                                           cubec_allocator_t allocator) {
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_literal_char_t
cubec_create_ast_literal_char(cubec_allocator_t allocator) {
  cubec_ast_literal_char_t chr =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_literal_char_t),
                            (cubec_dispose_fn_t)cubec_ast_literal_char_dispose);
  cubec_ast_node_initialize(allocator, &chr->super);
  chr->super.type = CUBEC_NODE_TYPE_LITERAL_CHAR;
  return chr;
}

cubec_ast_node_t cubec_read_ast_literal_char(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             cubec_position_t *end) {
  cubec_position_t current = *position;
  if (*current.offset != '\'') {
    return NULL;
  }
  current.offset++;
  current.column++;
  if (!*current.offset) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid charactor literal, missing '\''");
  }
  uint32_t code = cubec_ast_read_code(&current, end);
  if (U_FAILURE(code)) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid unicode code");
  }
  if (code == '\\') {
    if (!*current.offset) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid charactor literal, missing '\''");
    }
    code = cubec_ast_read_code(&current, end);
    if (U_FAILURE(code)) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
  } else if (code == '\n' || code == '\r' || code == 0x2028 || code == 0x2029) {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid charactor literal, missing '\''");
  }
  if (*current.offset != '\'') {
    return cubec_create_ast_error(allocator, *position, current,
                                  "Invalid charactor literal, missing '\''");
  }
  current.offset++;
  current.column++;
  cubec_ast_literal_char_t chr = cubec_create_ast_literal_char(allocator);
  chr->super.loc.begin = *position;
  chr->super.loc.end = current;
  *position = current;
  return &chr->super;
}