#include "astwriter/statement_if.h"
#include "astwriter/node.h"
cubec_any_t cubec_write_ast_statement_if(cubec_allocator_t allocator,
                                         cubec_ast_statement_if_t statement) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "condition",
                      cubec_write_ast_node(statement->condition, allocator));
  cubec_any_set_field(value, allocator, "consequent",
                      cubec_write_ast_node(statement->consequent, allocator));
  if (statement->alternate) {
    cubec_any_set_field(value, allocator, "alternate",
                        cubec_write_ast_node(statement->alternate, allocator));
  }
  return value;
}