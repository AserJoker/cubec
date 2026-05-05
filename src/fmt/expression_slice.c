#include "fmt/expression_slice.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_expression_slice(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t start = ast_get_child(node, "start");
  ast_node_t end = ast_get_child(node, "end");
  fmt_expression(allocator, host, stream);
  stream_write(stream, "[");
  if (start) {
    fmt_expression(allocator, start, stream);
  }
  stream_write(stream, " : ");
  if (end) {
    fmt_expression(allocator, end, stream);
  }
  stream_write(stream, "]");
}