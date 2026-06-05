#include "c/statement_expression.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"

void c_statement_expression(c_writer_t writer, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  c_expression(writer, expression);
  stream_write(writer->stream, ";");
  stream_newline(writer->stream);
}