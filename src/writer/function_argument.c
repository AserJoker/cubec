#include "writer/function_argument.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_function_argument(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "const");
  ast_node_t identifier = ast_get_child(node, "identifier");
  if (mut) {
    stream_write_location(stream, mut->loc);
    stream_write(stream, " ");
  }
  stream_write_location(stream, identifier->loc);
  stream_write(stream, ": ");
  write_expression(allocator, type, stream);
}