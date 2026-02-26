#include "astwriter/expression_member.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_expression_member(cubec_allocator_t allocator,
                                  cubec_ast_expression_member_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "host",
                      cubec_write_ast_node(self->host, allocator));
  cubec_any_set_field(value, allocator, "field",
                      cubec_write_ast_node(self->field, allocator));
  return value;
}