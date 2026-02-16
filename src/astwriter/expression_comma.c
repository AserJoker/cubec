#include "astwriter/expression_comma.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_expression_comma(cubec_allocator_t allocator,
                                 cubec_ast_expression_comma_t self) {

  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "current",
                        cubec_write_ast_node(self->current, allocator));
  cubec_value_set_field(value, allocator, "next",
                        cubec_write_ast_node(self->next, allocator));
  return value;
}