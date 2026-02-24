#include "astwriter/array_declarator.h"
#include "astwriter/node.h"

cubec_value_t
cubec_write_ast_array_declarator(cubec_allocator_t allocator,
                                 cubec_ast_array_declarator_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "length",
                        cubec_write_ast_node(self->length, allocator));
  cubec_value_set_field(value, allocator, "item_type",
                        cubec_write_ast_node(self->item_type, allocator));
  return value;
}