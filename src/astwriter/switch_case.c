#include "astwriter/switch_case.h"
#include "astwriter/node.h"

cubec_value_t cubec_write_ast_switch_case(cubec_allocator_t allocator,
                                          cubec_ast_switch_case_t cas) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  if (cas->condition) {
    cubec_value_set_field(value, allocator, "condition",
                          cubec_write_ast_node(cas->condition, allocator));
  }
  cubec_value_t statements = cubec_create_value(allocator);
  cubec_value_set_array(statements, allocator);
  cubec_list_node_t it = cubec_list_get_first(cas->statements);
  while (it != cubec_list_get_end(cas->statements)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(statements, allocator,
                       cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "statements", statements);
  return value;
}