#include "astwriter/statement_do_while.h"
#include "astwriter/node.h"
cubec_value_t
cubec_write_ast_statement_do_while(cubec_allocator_t allocator,
                                   cubec_ast_statement_do_while_t statement) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "condition",
                        cubec_write_ast_node(statement->condition, allocator));
  cubec_value_set_field(value, allocator, "body",
                        cubec_write_ast_node(statement->body, allocator));
  return value;
}