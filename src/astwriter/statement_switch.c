#include "astwriter/statement_switch.h"
#include "astwriter/node.h"

cubec_value_t
cubec_write_ast_statement_switch(cubec_allocator_t allocator,
                                 cubec_ast_statement_switch_t statement) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "condition",
                        cubec_write_ast_node(statement->condition, allocator));
  cubec_value_t cases =
      cubec_value_set_array(cubec_create_value(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(statement->cases);
  while (it != cubec_list_get_end(statement->cases)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(cases, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "cases", cases);
  return value;
}