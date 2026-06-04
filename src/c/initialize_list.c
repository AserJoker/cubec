#include "c/initialize_list.h"
#include "ast/node.h"
#include "c/expression.h"
#include "c/type.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/type.h"
void c_initialize_list(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  type_t t = *(type_t *)type->value->data;
  stream_write(stream, "((");
  c_type(writer, t);
  stream_write(stream, "){");
  if (t->kind == TYPE_KIND_ARRAY) {
    stream_inc_indent(stream);
    stream_newline(stream);
    stream_write(stream, ".items = {");
    if (ast_get_length(fields)) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        stream_newline(stream);
        c_expression(writer, field);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "},");
    stream_dec_indent(stream);
    stream_newline(stream);
  } else {
    if (ast_get_length(fields)) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        stream_newline(stream);
        ast_node_t field = ast_get_item(fields, idx);
        ast_node_t identifier = ast_get_child(field, "identifier");
        ast_node_t value = ast_get_child(field, "value");
        char *name = location_get(node_get_location(identifier), allocator);
        stream_write(stream, ".%s = ", name);
        allocator_free(allocator, name);
        c_expression(writer, value);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
  }
  stream_write(stream, "})");
}