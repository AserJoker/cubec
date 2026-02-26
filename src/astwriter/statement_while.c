#include "astwriter/statement_while.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_statement_while(cubec_allocator_t allocator,
                                cubec_ast_statement_while_t statement) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "condition",
                      cubec_write_ast_node(statement->condition, allocator));
  cubec_any_set_field(value, allocator, "body",
                      cubec_write_ast_node(statement->body, allocator));
  return value;
}