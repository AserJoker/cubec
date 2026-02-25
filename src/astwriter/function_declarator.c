#include "astwriter/function_declarator.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_function_declarator(cubec_allocator_t allocator,
                                    cubec_ast_function_declarator_t self) {
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
  if (self->kind) {
    cubec_value_set_field(value, allocator, "kind",
                          cubec_write_ast_node(self->kind, allocator));
  }
  cubec_value_t closure = cubec_create_value(allocator);
  cubec_value_set_array(closure, allocator);
  it = cubec_list_get_first(self->closure);
  while (it != cubec_list_get_end(self->closure)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(closure, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "closure", closure);
  if (self->identifier) {
    cubec_value_set_field(value, allocator, "identifier",
                          cubec_write_ast_node(self->identifier, allocator));
  }
  cubec_value_t args = cubec_create_value(allocator);
  cubec_value_set_array(args, allocator);
  it = cubec_list_get_first(self->args);
  while (it != cubec_list_get_end(self->args)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(args, allocator, cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "args", args);
  if (self->type) {
    cubec_value_set_field(value, allocator, "return_type",
                          cubec_write_ast_node(self->type, allocator));
  }
  cubec_value_set_field(value, allocator, "body",
                        cubec_write_ast_node(self->body, allocator));
  return value;
}