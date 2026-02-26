#include "astwriter/expression_binary.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_expression_binary(cubec_allocator_t allocator,
                                  cubec_ast_expression_binary_t self) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  if (self->left) {
    cubec_any_set_field(value, allocator, "left",
                        cubec_write_ast_node(self->left, allocator));
  }
  cubec_any_set_field(value, allocator, "opt",
                      cubec_write_ast_node(self->opt, allocator));
  if (self->right) {
    cubec_any_set_field(value, allocator, "right",
                        cubec_write_ast_node(self->right, allocator));
  }
  return value;
}