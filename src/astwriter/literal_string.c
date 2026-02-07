#include "astwriter/literal_string.h"

cubec_value_t
cubec_write_ast_literal_string(cubec_allocator_t allocator,
                               cubec_ast_literal_string_t literal_string) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  return value;
}