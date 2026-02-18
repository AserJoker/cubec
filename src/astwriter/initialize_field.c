#include "astwriter/initialize_field.h"
#include "astwriter/node.h"

cubec_value_t
cubec_write_ast_initialize_field(cubec_allocator_t allocator,
                                 cubec_ast_initialize_field_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  if (self->identifier) {
    cubec_value_set_field(value, allocator, "identifier",
                          cubec_write_ast_node(self->identifier, allocator));
  }
  cubec_value_set_field(value, allocator, "initialize",
                        cubec_write_ast_node(self->initialize, allocator));
  return value;
}