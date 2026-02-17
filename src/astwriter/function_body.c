#include "astwriter/function_body.h"
#include "astwriter/node.h"

cubec_value_t cubec_write_ast_function_body(cubec_allocator_t allocator,
                                            cubec_ast_function_body_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t statements =
      cubec_value_set_array(cubec_create_value(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(self->statements);
  while (it != cubec_list_get_end(self->statements)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(statements, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "statements", statements);
  return value;
}