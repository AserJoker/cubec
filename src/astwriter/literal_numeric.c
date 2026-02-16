#include "astwriter/literal_numeric.h"

cubec_value_t
cubec_write_ast_literal_numeric(cubec_allocator_t allocator,
                                cubec_ast_literal_numeric_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  return value;
}