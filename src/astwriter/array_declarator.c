#include "astwriter/array_declarator.h"
#include "astwriter/node.h"
cubec_value_t
cubec_write_ast_array_declarator(cubec_allocator_t allocator,
                                 cubec_ast_array_declarator_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t items = cubec_create_value(allocator);
  cubec_value_set_array(items, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->items);
  while (it != cubec_list_get_end(self->items)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(items, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "items", items);
  return value;
}