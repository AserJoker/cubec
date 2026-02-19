#include "astwriter/statement_function.h"
#include "astwriter/node.h"

cubec_value_t
cubec_write_ast_statement_function(cubec_allocator_t allocator,
                                   cubec_ast_statement_function_t statement) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "function",
                        cubec_write_ast_node(statement->function, allocator));
  return value;
}