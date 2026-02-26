#include "astwriter/expression_group.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_expression_group(cubec_allocator_t allocator,
                                 cubec_ast_expression_group_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "body",
                      cubec_write_ast_node(self->body, allocator));
  return value;
}