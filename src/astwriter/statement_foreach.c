#include "astwriter/statement_foreach.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_statement_foreach(cubec_allocator_t allocator,
                                  cubec_ast_statement_foreach_t statement) {

  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  if (statement->kind) {
    cubec_any_set_field(value, allocator, "kind",
                        cubec_write_ast_node(statement->kind, allocator));
  }
  cubec_any_set_field(value, allocator, "identifier",
                      cubec_write_ast_node(statement->identifier, allocator));
  cubec_any_set_field(value, allocator, "expression",
                      cubec_write_ast_node(statement->expression, allocator));
  cubec_any_set_field(value, allocator, "body",
                      cubec_write_ast_node(statement->body, allocator));
  return value;
}