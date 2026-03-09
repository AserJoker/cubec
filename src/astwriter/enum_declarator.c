#include "astwriter/enum_declarator.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_enum_declarator(cubec_allocator_t allocator,
                                            cubec_ast_enum_declarator_t self) {

  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
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
  if (self->identifier) {
    cubec_any_set_field(value, allocator, "identifier",
                        cubec_write_ast_node(self->identifier, allocator));
  }
  if (self->type) {
    cubec_any_set_field(value, allocator, "enum_type",
                        cubec_write_ast_node(self->type, allocator));
  }
  cubec_any_t fields = cubec_create_any(allocator);
  cubec_any_set_array(fields, allocator);
  it = cubec_list_get_first(self->fields);
  while (it != cubec_list_get_end(self->fields)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_any_append(fields, allocator, cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "fields", fields);
  return value;
}