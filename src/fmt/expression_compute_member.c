#include "fmt/expression_compute_member.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_expression_compute_member(allocator_t allocator, ast_node_t node,
                                   stream_t stream) {
  ast_node_t host_node = ast_get_child(node, "host");
  ast_node_t field_node = ast_get_child(node, "field");
  fmt_expression(allocator, host_node, stream);
  stream_write(stream, "[");
  fmt_expression(allocator, field_node, stream);
  stream_write(stream, "]");
}