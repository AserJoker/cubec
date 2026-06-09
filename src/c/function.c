#include "c/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/statement_block.h"
#include "c/type.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
#include <string.h>
void c_function_declar(c_writer_t writer, value_t func) {
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  stream_t stream = writer->stream;
  type_t type = func->type;
  function_meta_t meta = type->meta;
  function_declar_t declar = *(function_declar_t *)func->data;
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t accessor = ast_get_child(declar->node, "accessor");
  if (declar->kind == FUNCTION_KIND_EXTERN) {
    stream_write(stream, "extern ");
  } else if (!accessor || !location_is(node_get_location(accessor), "pub")) {
    stream_write(writer->stream, "static ");
  }
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
    if (ctype->type) {
      if (!ctype->mut) {
        stream_write(writer->stream, "const ");
      }
      c_type(writer, ctype->type);
      stream_write(writer->stream, " ");
      ast_node_t identifier = ast_get_child(arg, "identifier");
      char *name = location_get(node_get_location(identifier), allocator);
      stream_write(stream, name);
      allocator_free(allocator, name);
    } else {
      stream_write(stream, "...");
    }
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
  if (declar->kind == FUNCTION_KIND_EXTERN) {
    return;
  }
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t accessor = ast_get_child(declar->node, "accessor");
  if (!accessor || !location_is(node_get_location(accessor), "pub")) {
    stream_write(writer->stream, "static ");
  }
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
  stream_write(stream, ") ");
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

void c_function_closure(c_writer_t writer, value_t function) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  function_meta_t meta = function->type->meta;
  function_declar_t declar = *(function_declar_t *)function->data;
  if (declar->kind == FUNCTION_KIND_COMPTIME) {
    return;
  }
  if (hash_map_get_size(meta->closure)) {
    stream_write(stream, "((%s){", function->type->id);
    stream_inc_indent(stream);
    stream_newline(stream);
    stream_write(stream, ".callee = %s_fn,", declar->id);
    stream_newline(stream);
    stream_write(stream, ".env = {");
    stream_inc_indent(stream);
    list_node_t cit = hash_map_get_first(meta->closure);
    while (cit != hash_map_get_end(meta->closure)) {
      stream_newline(stream);
      const char *key = hash_map_node_get_key(cit);
      stream_write(stream, ".%s = %s,", key, key);
      cit = hash_map_node_get_next(cit);
    }
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "},");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "})");
  } else {
    stream_write(stream, declar->id);
  }
}
void c_closure_declar(c_writer_t writer, ast_node_t node) {
  if (node->type != NODE_TYPE_FUNCTION_DECLARATOR) {
    return;
  }
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t bind = ast_get_child(node, "bind");
  value_t function = bind->value;
  function_declar_t declar = *(function_declar_t *)function->data;
  if (function->type->kind == TYPE_KIND_TEMPLATE) {
    list_node_t it = hash_map_get_first(ctx->functions);
    while (it != hash_map_get_end(ctx->functions)) {
      value_t func = hash_map_node_get_value(it);
      function_declar_t fdeclar = *(function_declar_t *)func->data;
      if (strcmp(declar->id, fdeclar->template_id) == 0) {
        stream_write(stream, "%s %s = ", func->type->id, fdeclar->id);
        c_function_closure(writer, func);
        stream_write(stream, ";");
        stream_newline(stream);
      }
      it = hash_map_node_get_next(it);
    }
  } else {
    stream_write(stream, "%s %s = ", function->type->id, declar->id);
    c_function_closure(writer, bind->value);
    stream_write(stream, ";");
    stream_newline(stream);
  }
}