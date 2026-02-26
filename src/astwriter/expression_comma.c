#include "astwriter/expression_comma.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_expression_comma(cubec_allocator_t allocator,
                                 cubec_ast_expression_comma_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "current",
                      cubec_write_ast_node(self->current, allocator));
  cubec_any_set_field(value, allocator, "next",
                      cubec_write_ast_node(self->next, allocator));
  return value;
}