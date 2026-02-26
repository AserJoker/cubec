#include "astwriter/literal_symbol.h"

cubec_any_t
cubec_write_ast_literal_symbol(cubec_allocator_t allocator,
                               cubec_ast_literal_symbol_t literal_symbol) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  return value;
}