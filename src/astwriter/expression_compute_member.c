#include "astwriter/expression_compute_member.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_expression_compute_member(
    cubec_allocator_t allocator, cubec_ast_expression_compute_member_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "host",
                        cubec_write_ast_node(self->host, allocator));
  cubec_value_set_field(value, allocator, "field",
                        cubec_write_ast_node(self->field, allocator));
  return value;
}