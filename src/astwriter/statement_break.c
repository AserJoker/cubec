#include "astwriter/statement_break.h"

cubec_any_t cubec_write_ast_statement_break(cubec_allocator_t allocator,
                                            cubec_ast_statement_break_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  return value;
}