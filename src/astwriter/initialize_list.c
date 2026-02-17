#include "astwriter/initialize_list.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_initialize_list(cubec_allocator_t allocator,
                                cubec_ast_initialize_list_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t fields = cubec_create_value(allocator);
  cubec_value_set_array(fields, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->fields);
  while (it != cubec_list_get_end(self->fields)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(fields, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "fields", fields);
  return value;
}