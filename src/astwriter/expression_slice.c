#include "astwriter/expression_slice.h"
#include "astwriter/node.h"

cubec_value_t
cubec_write_ast_expression_slice(cubec_allocator_t allocator,
                                 cubec_ast_expression_slice_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "host",
                        cubec_write_ast_node(self->host, allocator));
  if (self->start) {
    cubec_value_set_field(value, allocator, "start",
                          cubec_write_ast_node(self->start, allocator));
  }
  if (self->end) {
    cubec_value_set_field(value, allocator, "end",
                          cubec_write_ast_node(self->end, allocator));
  }
  return value;
}