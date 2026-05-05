#include "fmt/function_argument_rest.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_function_argument_rest(allocator_t allocator, ast_node_t node,
                                stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t identifier = ast_get_child(node, "identifier");
  stream_write(stream, "...");
  if (identifier) {
    stream_write_location(stream, identifier->loc);
    stream_write(stream, ": ");
  }
  if (mut) {
    stream_write_location(stream, mut->loc);
    stream_write(stream, " ");
  }
  fmt_expression(allocator, type, stream);
}