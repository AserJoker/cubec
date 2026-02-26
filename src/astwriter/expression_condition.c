#include "astwriter/expression_condition.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_expression_condition(cubec_allocator_t allocator,
                                     cubec_ast_expression_condition_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "condition",
                      cubec_write_ast_node(self->condition, allocator));
  cubec_any_set_field(value, allocator, "consequent",
                      cubec_write_ast_node(self->consequent, allocator));
  cubec_any_set_field(value, allocator, "alternate",
                      cubec_write_ast_node(self->alternate, allocator));
  return value;
}