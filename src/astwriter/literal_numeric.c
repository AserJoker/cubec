#include "astwriter/literal_numeric.h"

cubec_any_t cubec_write_ast_literal_numeric(cubec_allocator_t allocator,
                                            cubec_ast_literal_numeric_t self) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  return value;
}