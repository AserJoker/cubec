#include "writer/expression_spread.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"

void write_expression_spread(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  stream_write(stream, "...");
  ast_node_t expression = ast_get_child(node, "expression");
  write_expression(allocator, expression, stream);
}