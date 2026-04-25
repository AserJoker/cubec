#include "writer/initialize_field.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_initialize_field(allocator_t allocator, ast_node_t node,
                            stream_t stream) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t initialize = ast_get_child(node, "initialize");
  if (identifier) {
    stream_write(stream, ".");
    stream_write_location(stream, identifier->loc);
    stream_write(stream, " = ");
  }
  write_expression(allocator, initialize, stream);
}