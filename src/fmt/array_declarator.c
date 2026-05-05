#include "fmt/array_declarator.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_array_declarator(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t length = ast_get_child(node, "length");
  ast_node_t type = ast_get_child(node, "type");
  stream_write(stream, "[");
  fmt_expression(allocator, length, stream);
  stream_write(stream, "]");
  fmt_expression(allocator, type, stream);
}