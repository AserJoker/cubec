#include "astwriter/statement_expression.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_statement_expression(
    cubec_allocator_t allocator, cubec_ast_statement_expression_t statement) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "expression",
                        cubec_write_ast_node(statement->expression, allocator));
  return value;
}