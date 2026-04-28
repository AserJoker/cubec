#include "writer/expression_assigment.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_expression_assigment(allocator_t allocator, ast_node_t node,
                                stream_t stream) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t opt = ast_get_child(node, "opt");
  ast_node_t value = ast_get_child(node, "value");
  write_expression(allocator, identifier, stream);
  stream_write(stream, " ");
  stream_write_location(stream, opt->loc);
  stream_write(stream, " ");
  write_expression(allocator, value, stream);
}