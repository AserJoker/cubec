#include "astwriter/variable_declarator.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_variable_declarator(cubec_allocator_t allocator,
                                    cubec_ast_variable_declarator_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "identifier",
                      cubec_write_ast_node(self->identifier, allocator));
  if (self->type) {
    cubec_any_set_field(value, allocator, "variable_type",
                        cubec_write_ast_node(self->type, allocator));
  }
  if (self->initialize) {
    cubec_any_set_field(value, allocator, "initialize",
                        cubec_write_ast_node(self->initialize, allocator));
  }
  return value;
}