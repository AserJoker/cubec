#include "astwriter/statement_struct.h"
#include "astwriter/node.h"
cubec_any_t
cubec_write_ast_statement_struct(cubec_allocator_t allocator,
                                 cubec_ast_statement_struct_t statement) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "struct",
                      cubec_write_ast_node(statement->stru, allocator));
  return value;
}