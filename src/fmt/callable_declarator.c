#include "fmt/callable_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/stream.h"
#include "fmt/expression.h"
#include "fmt/function_argument.h"
#include "fmt/function_argument_rest.h"
void fmt_callable_declarator(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t args = ast_get_child(node, "arguments");
  stream_write(stream, "func(");
  for (size_t idx = 0; idx < ast_get_length(args); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(args, idx);
    if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT) {
      fmt_function_argument(allocator, arg, stream);
    } else {
      fmt_function_argument_rest(allocator, arg, stream);
    }
  }
  stream_write(stream, ") -> ");
  fmt_expression(allocator, type, stream);
}