#include "astwriter/statement_return.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_statement_return(cubec_allocator_t allocator,
                                 cubec_ast_statement_return_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  if (self->value) {
    cubec_value_set_field(value, allocator, "value",
                          cubec_write_ast_node(self->value, allocator));
  }
  return value;
}
