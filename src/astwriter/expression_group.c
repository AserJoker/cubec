#include "astwriter/expression_group.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_expression_group(cubec_allocator_t allocator,
                                 cubec_ast_expression_group_t self) {

  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "body",
                        cubec_write_ast_node(self->body, allocator));
  return value;
}