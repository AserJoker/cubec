#include "astwriter/function_body.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_function_body(cubec_allocator_t allocator,
                                          cubec_ast_function_body_t self) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  cubec_any_t statements =
      cubec_any_set_array(cubec_create_any(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(self->statements);
  while (it != cubec_list_get_end(self->statements)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_any_t item = cubec_write_ast_node(node, allocator);
    cubec_any_append(statements, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "statements", statements);
  return value;
}