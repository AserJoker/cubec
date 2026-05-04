#include "fmt/expression_spread.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"

void fmt_expression_spread(allocator_t allocator, ast_node_t node,
                           stream_t stream) {
  stream_write(stream, "...");
  ast_node_t expression = ast_get_child(node, "expression");
  fmt_expression(allocator, expression, stream);
}