#include "astwriter/statement_expression.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t cubec_write_ast_statement_expression(
    cubec_allocator_t allocator, cubec_ast_statement_expression_t statement) {
  cubec_any_t value = cubec_create_any(allocator);
  cubec_any_set_object(value, allocator);
  cubec_any_set_field(value, allocator, "expression",
                      cubec_write_ast_node(statement->expression, allocator));
  return value;
}