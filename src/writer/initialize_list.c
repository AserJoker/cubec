#include "writer/initialize_list.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
#include "writer/initialize_field.h"
void write_initialize_list(allocator_t allocator, ast_node_t node,
                           stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  if (type) {
    write_expression(allocator, type, stream);
  }
  stream_write(stream, "{");
  if (ast_get_length(fields)) {
    stream_inc_indent(stream);
    stream_newline(stream);
    for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
      if (idx != 0) {
        stream_write(stream, ",");
        stream_newline(stream);
      }
      ast_node_t field = ast_get_item(fields, idx);
      write_initialize_field(allocator, field, stream);
    }
    stream_dec_indent(stream);
    stream_newline(stream);
  }
  stream_write(stream, "}");
}