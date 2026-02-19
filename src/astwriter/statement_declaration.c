#include "astwriter/statement_declaration.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_statement_declaration(cubec_allocator_t allocator,
                                      cubec_ast_statement_declaration_t self) {

  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
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
  cubec_value_set_field(value, allocator, "kind",
                        cubec_write_ast_node(self->kind, allocator));
  cubec_value_t declarations =
      cubec_value_set_array(cubec_create_value(allocator), allocator);
  it = cubec_list_get_first(self->declarations);
  while (it != cubec_list_get_end(self->declarations)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(declarations, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "declarations", declarations);
  return value;
}