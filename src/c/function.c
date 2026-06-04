#include "c/function.h"
#include "ast/node.h"
#include "c/statement_block.h"
#include "c/type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
void c_function_declar(c_writer_t writer, value_t func) {
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  stream_t stream = writer->stream;
  type_t type = func->type;
  function_meta_t meta = type->meta;
  function_declar_t declar = *(function_declar_t *)func->data;
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  if (!meta->type->mut) {
    stream_write(writer->stream, "const ");
  }
  c_type(writer, meta->type->type);
  stream_write(writer->stream, " ");
  stream_write(writer->stream, declar->id);
  if (hash_map_get_size(meta->closure)) {
    stream_write(writer->stream, "_fn");
  }
  stream_write(writer->stream, "(");
  if (hash_map_get_size(meta->closure)) {
    stream_write(stream, "%s_env *__env__", type->id);
    if (array_get_size(meta->args)) {
      stream_write(stream, ", ");
    }
  }
  for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(arguments, idx);
    ctype_t ctype = array_get(meta->args, idx);
    if (!ctype->mut) {
      stream_write(writer->stream, "const ");
    }
    c_type(writer, ctype->type);
    stream_write(writer->stream, " ");
    ast_node_t identifier = ast_get_child(arg, "identifier");
    char *name = location_get(node_get_location(identifier), allocator);
    stream_write(stream, name);
    allocator_free(allocator, name);
  }
  stream_write(stream, ");");
  stream_newline(stream);
}
void c_function_declaration(c_writer_t writer, value_t func) {
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  stream_t stream = writer->stream;
  type_t type = func->type;
  function_meta_t meta = type->meta;
  function_declar_t declar = *(function_declar_t *)func->data;
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  if (!meta->type->mut) {
    stream_write(writer->stream, "const ");
  }
  c_type(writer, meta->type->type);
  stream_write(writer->stream, " ");
  stream_write(writer->stream, declar->id);
  if (hash_map_get_size(meta->closure)) {
    stream_write(writer->stream, "_fn");
  }
  stream_write(writer->stream, "(");
  if (hash_map_get_size(meta->closure)) {
    stream_write(stream, "%s_env *__env__", type->id);
    if (array_get_size(meta->args)) {
      stream_write(stream, ", ");
    }
  }
  for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(arguments, idx);
    ctype_t ctype = array_get(meta->args, idx);
    if (!ctype->mut) {
      stream_write(writer->stream, "const ");
    }
    c_type(writer, ctype->type);
    stream_write(writer->stream, " ");
    ast_node_t identifier = ast_get_child(arg, "identifier");
    char *name = location_get(node_get_location(identifier), allocator);
    stream_write(stream, name);
    allocator_free(allocator, name);
  }
  stream_write(stream, ")");
  stream_newline(stream);
  module_t mod = ctx->mod;
  ctx->mod = declar->mod;
  type_t global = ctx->global;
  ctx->global = *(type_t *)declar->mod->value->data;
  type_t self = ctx->self;
  ctx->self = declar->self;
  value_t function = ctx->function;
  ctx->function = func;
  context_type_t ctx_type = ctx->type;
  ctx->type = CONTEXT_TYPE_FUNCTION;
  bool comptime = ctx->comptime;
  ctx->comptime = false;
  ast_node_t body = ast_get_child(declar->node, "body");
  c_statement_block(writer, body);
  ctx->comptime = comptime;
  ctx->type = ctx_type;
  ctx->self = self;
  ctx->global = global;
  ctx->mod = mod;
}