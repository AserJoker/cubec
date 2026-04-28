#include "writer/expression_compute_member.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_expression_compute_member(allocator_t allocator, ast_node_t node,
                                     stream_t stream) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field_node = ast_get_child(node, "field");
  write_expression(allocator, host_node, stream);
  stream_write(stream, "[");
  write_expression(allocator, field_node, stream);
  stream_write(stream, "]");
}