#include "astwriter/union_declarator.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_union_declarator(cubec_allocator_t allocator,
                                 cubec_ast_union_declarator_t self) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_t types =
      cubec_any_set_array(cubec_create_any(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(self->types);
  while (it != cubec_list_get_end(self->types)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_any_t item = cubec_write_ast_node(node, allocator);
    cubec_any_append(types, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "types", types);
  return value;
}