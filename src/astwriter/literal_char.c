#include "astwriter/literal_char.h"

cubec_value_t
cubec_write_ast_literal_char(cubec_allocator_t allocator,
                             cubec_ast_literal_char_t literal_char) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  return value;
}