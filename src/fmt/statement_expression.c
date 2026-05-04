#include "fmt/statement_expression.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_statement_expression(allocator_t allocator, ast_node_t node,
                              stream_t stream) {
  ast_node_t expression = ast_get_child(node, "expression");
  fmt_expression(allocator, expression, stream);
  stream_write(stream, ";");
}