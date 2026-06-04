#include "c/expression_binary.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"
void c_expression_binary(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  ast_node_t left = ast_get_child(node, "left");
  ast_node_t right = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (left) {
    c_expression(writer, left);
  }
  stream_write(stream, " ");
  stream_write_location(stream, node_get_location(opt));
  stream_write(stream, " ");
  c_expression(writer, right);
}
