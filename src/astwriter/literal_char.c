#include "astwriter/literal_char.h"

cubec_any_t
cubec_write_ast_literal_char(cubec_allocator_t allocator,
                             cubec_ast_literal_char_t literal_char) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  return value;
}