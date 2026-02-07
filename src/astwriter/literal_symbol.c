#include "astwriter/literal_symbol.h"

cubec_value_t
cubec_write_ast_literal_symbol(cubec_allocator_t allocator,
                               cubec_ast_literal_symbol_t literal_symbol) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  return value;
}