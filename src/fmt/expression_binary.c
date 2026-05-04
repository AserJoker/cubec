#include "fmt/expression_binary.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_expression_binary(allocator_t allocator, ast_node_t node,
                           stream_t stream) {
  ast_node_t left_node = ast_get_child(node, "left");
  ast_node_t right_node = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (left_node) {
    fmt_expression(allocator, left_node, stream);
    stream_write(stream, " ");
  }
  stream_write_location(stream, opt->loc);
  if (left_node) {
    stream_write(stream, " ");
  }
  fmt_expression(allocator, right_node, stream);
}