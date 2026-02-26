#include "astwriter/statement_enum.h"
#include "astwriter/node.h"

cubec_any_t
cubec_write_ast_statement_enum(cubec_allocator_t allocator,
                               cubec_ast_statement_enum_t statement) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "enum",
                      cubec_write_ast_node(statement->enu, allocator));
  return value;
}