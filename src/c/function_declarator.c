#include "c/function_declarator.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
void c_function_declarator(c_writer_t writer, ast_node_t node) {
  ast_node_t bind = ast_get_child(node, "bind");
  ast_node_t closure = ast_get_child(node, "closure");
  value_t func = bind->value;
  function_declar_t declar = *(function_declar_t *)func->data;
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  stream_write(stream, "((%s){", func->type->id);
  stream_inc_indent(stream);
  stream_newline(stream);
  stream_write(stream, ".callee = %s_fn,", declar->id);
  stream_newline(stream);
  stream_write(stream, ".env = {", declar->id);
  stream_inc_indent(stream);
  for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
    stream_newline(stream);
    ast_node_t item = ast_get_item(closure, idx);
    char *id = location_get(node_get_location(item), allocator);
    stream_write(stream, ".%s = ", id);
    allocator_free(allocator, id);
    c_expression(writer, item);
    stream_write(stream, ",");
  }
  stream_dec_indent(stream);
  stream_newline(stream);
  stream_write(stream, "},", declar->id);
  stream_dec_indent(stream);
  stream_newline(stream);
  stream_write(stream, "})");
}
