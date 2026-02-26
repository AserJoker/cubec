#include "astwriter/expression_slice.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_expression_slice(cubec_allocator_t allocator,
                                 cubec_ast_expression_slice_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "host",
                      cubec_write_ast_node(self->host, allocator));
  if (self->start) {
    cubec_any_set_field(value, allocator, "start",
                        cubec_write_ast_node(self->start, allocator));
  }
  if (self->end) {
    cubec_any_set_field(value, allocator, "end",
                        cubec_write_ast_node(self->end, allocator));
  }
  return value;
}