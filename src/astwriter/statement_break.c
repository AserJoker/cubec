#include "astwriter/statement_break.h"

cubec_value_t
cubec_write_ast_statement_break(cubec_allocator_t allocator,
                                cubec_ast_statement_break_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  return value;
}