#include "astwriter/statement_defer.h"
#include "astwriter/node.h"
#include "core/any.h"
cubec_any_t cubec_write_ast_statement_defer(cubec_allocator_t allocator,
                                            cubec_ast_statement_defer_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "statement",
                      cubec_write_ast_node(self->statement, allocator));
  return value;
}