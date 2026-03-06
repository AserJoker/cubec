#include "astwriter/type.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_type(cubec_allocator_t allocator,
                                 cubec_ast_type_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "expression",
                      cubec_write_ast_node(self->expression, allocator));
  return value;
}