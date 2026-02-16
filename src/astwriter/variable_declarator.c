#include "astwriter/variable_declarator.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_variable_declarator(cubec_allocator_t allocator,
                                    cubec_ast_variable_declarator_t self) {

  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "identifier",
                        cubec_write_ast_node(self->identifier, allocator));
  if (self->type) {
    cubec_value_set_field(value, allocator, "type",
                          cubec_write_ast_node(self->type, allocator));
  }
  if (self->initialize) {
    cubec_value_set_field(value, allocator, "initialize",
                          cubec_write_ast_node(self->initialize, allocator));
  }
  return value;
}