#include "astwriter/literal_string.h"

cubec_any_t
cubec_write_ast_literal_string(cubec_allocator_t allocator,
                               cubec_ast_literal_string_t literal_string) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  return value;
}