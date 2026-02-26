#include "astwriter/expression_assigment.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_expression_assigment(cubec_allocator_t allocator,
                                     cubec_ast_expression_assigment_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "identifier",
                      cubec_write_ast_node(self->identifier, allocator));
  cubec_any_set_field(value, allocator, "value",
                      cubec_write_ast_node(self->value, allocator));
  cubec_any_set_field(value, allocator, "opt",
                      cubec_write_ast_node(self->opt, allocator));
  return value;
}