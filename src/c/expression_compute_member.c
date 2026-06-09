#include "c/expression_compute_member.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
void c_expression_compute_member(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  if (host->vtype->kind == TYPE_KIND_STRUCT) {
    ast_node_t bind = ast_get_child(node, "bind");
    value_t value = bind->value;
    function_declar_t declar = *(function_declar_t *)value->data;
    if (hash_map_get_size(declar->closure)) {
      stream_write(stream, "%s_call(&%s, &(", value->type->id, declar->id);
      c_expression(writer, host);
      stream_write(stream, "), ");
      c_expression(writer, field);
      stream_write(stream, ")");
    } else {
      stream_write(stream, "%s(&(", declar->id);
      c_expression(writer, host);
      stream_write(stream, "), ");
      c_expression(writer, field);
      stream_write(stream, ")");
    }
  } else if (host->vtype->kind == TYPE_KIND_PTR) {
    ast_node_t bind = ast_get_child(node, "bind");
    value_t value = bind->value;
    function_declar_t declar = *(function_declar_t *)value->data;
    stream_write(stream, "%s(", declar->id);
    c_expression(writer, host);
    stream_write(stream, ", ");
    c_expression(writer, field);
    stream_write(stream, ")");
  } else if (host->vtype->kind == TYPE_KIND_PARRAY) {
    c_expression(writer, host);
    stream_write(stream, "[");
    c_expression(writer, field);
    stream_write(stream, "]");
  } else if (host->vtype->kind == TYPE_KIND_ARRAY) {
    c_expression(writer, host);
    stream_write(stream, ".items[");
    c_expression(writer, field);
    stream_write(stream, "]");
  } else if (host->vtype->kind == TYPE_KIND_SLICE) {
    stream_write(stream, "%s_get(", host->vtype->id);
    c_expression(writer, host);
    stream_write(stream, ", ");
    c_expression(writer, field);
    stream_write(stream, ")");
  }
}