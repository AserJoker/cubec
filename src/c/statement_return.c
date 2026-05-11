#include "c/statement_return.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"

void write_c_statement_return(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t value = ast_get_child(node, "value");
  stream_write(stream, "return");
  if (value) {
    stream_write(stream, " ");
    write_c_expression(ctx, value, stream);
  }
  stream_write(stream, ";");
}