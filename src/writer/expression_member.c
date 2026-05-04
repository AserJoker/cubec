#include "writer/expression_member.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_expression_member(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  if (host) {
    write_expression(allocator, host, stream);
  }
  stream_write(stream, ".");
  stream_write_location(stream, field->loc);
}