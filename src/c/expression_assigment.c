#include "c/expression_assigment.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/expression.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
void c_expression_assigment(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t left = ast_get_child(node, "left");
  ast_node_t right = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (left->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host = ast_get_child(left, "host");
    ast_node_t field = ast_get_child(left, "field");
    if (node_location_is(field, "*")) {
      stream_write(stream, "*");
      c_expression(writer, host);
    } else {
      c_expression(writer, left);
    }
    stream_write(stream, " ");
    stream_write_location(stream, node_get_location(opt));
    stream_write(stream, " ");
    c_expression(writer, right);
    return;
  } else if (left->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    ast_node_t host = ast_get_child(left, "host");
    ast_node_t field = ast_get_child(left, "field");
    if (host->vtype->kind == TYPE_KIND_ARRAY) {
      c_expression(writer, left);
      stream_write(stream, " ");
      stream_write_location(stream, node_get_location(opt));
      stream_write(stream, " ");
      c_expression(writer, right);
      return;
    } else if (host->vtype->kind == TYPE_KIND_SLICE) {
      stream_write(stream, "%s_set(", host->vtype->id);
      c_expression(writer, host);
      stream_write(stream, ", ");
      c_expression(writer, field);
      stream_write(stream, ", ");
      c_expression(writer, right);
      stream_write(stream, ")");
      return;
    } else if (host->vtype->kind == TYPE_KIND_STRUCT) {
      ast_node_t bind = ast_get_child(node, "bind");
      value_t func = bind->value;
      function_declar_t declar = *(function_declar_t *)func->data;
      if (hash_map_get_size(declar->closure)) {
        stream_write(stream, "%s_call(&%s, ", func->type->id, declar->id);
      } else {
        stream_write(stream, "%s(", declar->id);
      }
      stream_write(stream, "&(");
      c_expression(writer, host);
      stream_write(stream, "), ");
      c_expression(writer, field);
      stream_write(stream, ", ");
      if (node_location_is(opt, "=")) {
        c_expression(writer, right);
      } else {
        // TODO:
      }
      stream_write(stream, ")");
      return;
    }
  } else if (left->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    if (node_location_is(left, "_")) {
      stream_write(stream, "(void)(");
      c_expression(writer, right);
      stream_write(stream, ")");
      return;
    } else {
      c_expression(writer, left);
      stream_write(stream, " ");
      stream_write_location(stream, node_get_location(opt));
      c_expression(writer, right);
      return;
    }
  }
}