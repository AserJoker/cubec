#include "fmt/statement_return.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/expression.h"
void fmt_statement_return(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t value = ast_get_child(node, "value");
  stream_write(stream, "return");
  if (value) {
    stream_write(stream, " ");
    fmt_expression(allocator, value, stream);
  }
  stream_write(stream, ";");
}