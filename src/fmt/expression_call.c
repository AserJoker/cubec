#include "fmt/expression_call.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_expression_call(allocator_t allocator, ast_node_t node,
                         stream_t stream) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  fmt_expression(allocator, callee, stream);
  stream_write(stream, "(");
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(arguments, idx);
    fmt_expression(allocator, arg, stream);
  }
  stream_write(stream, ")");
}