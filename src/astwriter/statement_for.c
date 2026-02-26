#include "astwriter/statement_for.h"
#include "astwriter/node.h"

cubec_any_t cubec_write_ast_statement_for(cubec_allocator_t allocator,
                                          cubec_ast_statement_for_t statement) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "init",
                      cubec_write_ast_node(statement->init, allocator));
  cubec_any_set_field(value, allocator, "condition",
                      cubec_write_ast_node(statement->condition, allocator));
  if (statement->after) {
    cubec_any_set_field(value, allocator, "after",
                        cubec_write_ast_node(statement->after, allocator));
  }
  cubec_any_set_field(value, allocator, "body",
                      cubec_write_ast_node(statement->body, allocator));
  return value;
}