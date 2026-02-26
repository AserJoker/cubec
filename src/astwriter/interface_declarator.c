#include "astwriter/interface_declarator.h"
#include "astwriter/node.h"
#include "core/list.h"

cubec_any_t
cubec_write_ast_interface_declarator(cubec_allocator_t allocator,
                                     cubec_ast_interface_declarator_t self) {

  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  cubec_any_t args = cubec_create_any(allocator);
  cubec_any_set_array(args, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->args);
  while (it != cubec_list_get_end(self->args)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_any_append(args, allocator, cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "args", args);
  cubec_any_set_field(value, allocator, "return_type",
                      cubec_write_ast_node(self->return_type, allocator));
  return value;
}