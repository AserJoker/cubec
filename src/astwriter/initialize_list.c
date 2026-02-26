#include "astwriter/initialize_list.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t cubec_write_ast_initialize_list(cubec_allocator_t allocator,
                                            cubec_ast_initialize_list_t self) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  cubec_any_t fields = cubec_create_any(allocator);
  cubec_any_set_array(fields, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->fields);
  while (it != cubec_list_get_end(self->fields)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_any_t item = cubec_write_ast_node(node, allocator);
    cubec_any_append(fields, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "fields", fields);
  return value;
}