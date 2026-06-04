#include "c/statement_return.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"

void c_statement_return(c_writer_t writer, ast_node_t node) {
  ast_node_t value = ast_get_child(node, "value");
  stream_t stream = writer->stream;
  stream_write(stream, "return");
  if (value) {
    stream_write(stream, " ");
    c_expression(writer, value);
  }
  stream_write(stream, ";");
  stream_newline(stream);
}