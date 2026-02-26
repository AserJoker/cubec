#include "astwriter/struct_field.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_struct_field(cubec_allocator_t allocator,
                                         cubec_ast_struct_field_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_t decorators = cubec_create_any(allocator);
  cubec_any_set_array(decorators, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->decorators);
  while (it != cubec_list_get_end(self->decorators)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_any_append(decorators, allocator,
                     cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "decorators", decorators);
  cubec_any_set_field(value, allocator, "declarator",
                      cubec_write_ast_node(self->declarator, allocator));
  return value;
}