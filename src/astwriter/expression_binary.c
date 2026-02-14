#include "astwriter/expression_binary.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_expression_binary(cubec_allocator_t allocator,
                                  cubec_ast_expression_binary_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "left",
                        cubec_write_ast_node(self->left, allocator));
  cubec_value_set_field(value, allocator, "opt",
                        cubec_write_ast_node(self->opt, allocator));
  cubec_value_set_field(value, allocator, "right",
                        cubec_write_ast_node(self->right, allocator));
  return value;
}