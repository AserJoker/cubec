#include "fmt/variable_declarator.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "fmt/expression.h"
void fmt_variable_declarator(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t initialize = ast_get_child(node, "initialize");
  ast_node_t identifier = ast_get_child(node, "identifier");
  stream_write_location(stream, identifier->loc);
  if (type) {
    stream_write(stream, ": ");
    fmt_expression(allocator, type, stream);
  }
  if (initialize) {
    stream_write(stream, " = ");
    fmt_expression(allocator, initialize, stream);
  }
}