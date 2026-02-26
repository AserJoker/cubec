#include "astwriter/expression_call.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_expression_call(cubec_allocator_t allocator,
                                            cubec_ast_expression_call_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "callee",
                      cubec_write_ast_node(self->callee, allocator));
  cubec_any_t args = cubec_create_any(allocator);
  cubec_any_set_array(args, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->args);
  while (it != cubec_list_get_end(self->args)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_any_append(args, allocator, cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "args", args);
  return value;
}