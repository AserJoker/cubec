#include "c/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/expression.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

void c_expression_call(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t callee = ast_get_child(node, "callee");
  value_t func = NULL;
  if (callee->type == NODE_TYPE_VALUE) {
    func = callee->value;
  } else {
    func = ast_get_child(callee, "bind")->value;
  }
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t host = NULL;
  ast_node_t field = NULL;
  if (callee->type == NODE_TYPE_EXPRESSION_MEMBER) {
    host = ast_get_child(callee, "host");
    field = ast_get_child(callee, "field");
  }
  type_t type = func->type;
  function_meta_t meta = type->meta;
  function_declar_t declar = *(function_declar_t *)func->data;
  if (host && host->vtype->kind == TYPE_KIND_STRUCT) {
    type_t type = host->vtype;
    if (!hash_map_get_size(meta->closure)) {
      stream_write(stream, "%s", declar->id);
      stream_write(stream, "(");
    } else {
      stream_write(stream, "%s_call(", func->type->id);
      stream_write(stream, "&%s", declar->id);
      stream_write(stream, ", ");
    }
    stream_write(stream, "&");
    c_expression(writer, host);
    if (ast_get_length(arguments)) {
      stream_write(stream, ", ");
    }
  } else {
    if (!hash_map_get_size(meta->closure)) {
      c_expression(writer, callee);
      stream_write(stream, "(");
    } else {
      stream_write(stream, "%s_call(&(", type->id);
      c_expression(writer, callee);
      stream_write(stream, ")");
      if (ast_get_length(arguments)) {
        stream_write(stream, ", ");
      }
    }
  }
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(arguments, idx);
    c_expression(writer, arg);
  }
  stream_write(stream, ")");
}