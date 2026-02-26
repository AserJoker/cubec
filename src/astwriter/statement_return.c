#include "astwriter/statement_return.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_statement_return(cubec_allocator_t allocator,
                                 cubec_ast_statement_return_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  if (self->value) {
    cubec_any_set_field(value, allocator, "value",
                        cubec_write_ast_node(self->value, allocator));
  }
  return value;
}
