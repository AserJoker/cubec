#include "c/expression_member.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/type.h"

void c_expression_member(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;

  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  if (node_location_is(field, "&")) {
    stream_write(stream, "&");
    c_expression(writer, host);
  } else if (node_location_is(field, "*")) {
    stream_write(stream, "*");
    c_expression(writer, host);
  } else {
    c_expression(writer, host);
    if (host->vtype->kind == TYPE_KIND_PTR) {
      stream_write(stream, "->");
    } else {
      stream_write(stream, ".");
    }
    stream_write_location(stream, node_get_location(field));
  }
}