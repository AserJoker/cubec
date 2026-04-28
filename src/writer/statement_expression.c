#include "writer/statement_expression.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_statement_expression(allocator_t allocator, ast_node_t node,
                                stream_t stream) {
  ast_node_t expression = ast_get_child(node, "expression");
  write_expression(allocator, expression, stream);
  stream_write(stream, ";");
}