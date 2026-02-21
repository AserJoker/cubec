#include "astwriter/statement_continue.h"

cubec_value_t
cubec_write_ast_statement_continue(cubec_allocator_t allocator,
                                   cubec_ast_statement_continue_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  return value;
}