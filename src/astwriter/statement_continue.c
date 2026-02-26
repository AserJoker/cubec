#include "astwriter/statement_continue.h"

cubec_any_t
cubec_write_ast_statement_continue(cubec_allocator_t allocator,
                                   cubec_ast_statement_continue_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  return value;
}