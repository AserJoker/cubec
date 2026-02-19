#include "astwriter/struct_declarator.h"
#include "astwriter/node.h"
#include "core/value.h"
cubec_value_t
cubec_write_ast_struct_declarator(cubec_allocator_t allocator,
                                  cubec_ast_struct_declarator_t self) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t decorators = cubec_create_value(allocator);
  cubec_value_set_array(decorators, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->decorators);
  while (it != cubec_list_get_end(self->decorators)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(decorators, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "decorators", decorators);
  cubec_value_set_field(value, allocator, "identifier",
                        cubec_write_ast_node(self->identifier, allocator));
  cubec_value_t fields = cubec_create_value(allocator);
  cubec_value_set_array(fields, allocator);
  it = cubec_list_get_first(self->fields);
  while (it != cubec_list_get_end(self->fields)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(fields, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "fields", fields);
  cubec_value_t methods = cubec_create_value(allocator);
  cubec_value_set_array(methods, allocator);
  it = cubec_list_get_first(self->methods);
  while (it != cubec_list_get_end(self->methods)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(methods, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "methods", methods);
  cubec_value_t attributes = cubec_create_value(allocator);
  cubec_value_set_array(attributes, allocator);
  it = cubec_list_get_first(self->attributes);
  while (it != cubec_list_get_end(self->attributes)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(attributes, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "attributes", attributes);
  return value;
}