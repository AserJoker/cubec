#include "ast/node.h"
#include "c/function_body.h"
#include "c/function_declarator.h"
#include "c/type.h"
#include "core/array.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/function.h"
#include "engine/value.h"
#include <string.h>

void write_c_function_declarator(context_t ctx, value_t value,
                                 stream_t stream) {
  function_declar_t declar = *(function_declar_t *)value_get_data(value);
  if (declar->type == FUNC_TYPE_NATIVE) {
    return;
  }
  ast_node_t node = declar->node;
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  if (kind && location_is(kind->loc, "comptime")) {
    return;
  }
  type_t type = value_get_type(value);
  ctype_t result_type = function_type_get_type(type);
  array_t arguments = function_type_get_arguments(type);
  if (!result_type->mut) {
    stream_write(stream, "const ");
  }
  write_c_type(ctx, result_type->type, stream);
  stream_write(stream, " ");
  stream_write(stream, declar->id);
  stream_write(stream, "(");
  for (size_t idx = 0; idx < ast_get_length(arguments_node); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg_node = ast_get_item(arguments_node, idx);
    ast_node_t identifier = ast_get_child(arg_node, "identifier");
    ctype_t ctype = array_get(arguments, idx);
    if (!ctype->mut) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, ctype->type, stream);
    stream_write(stream, " ");
    stream_write_location(stream, identifier->loc);
  }
  stream_write(stream, ");");
  stream_newline(stream);
}
void write_c_function_declaration(context_t ctx, value_t value,
                                  stream_t stream) {
  function_declar_t declar = *(function_declar_t *)value_get_data(value);
  if (declar->type == FUNC_TYPE_NATIVE) {
    return;
  }
  ast_node_t node = declar->node;
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t body = ast_get_child(node, "body");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  if (kind && location_is(kind->loc, "comptime")) {
    return;
  }
  type_t type = value_get_type(value);
  ctype_t result_type = function_type_get_type(type);
  array_t arguments = function_type_get_arguments(type);
  if (!result_type->mut) {
    stream_write(stream, "const ");
  }
  write_c_type(ctx, result_type->type, stream);
  stream_write(stream, " ");
  stream_write(stream, declar->id);
  stream_write(stream, "(");
  for (size_t idx = 0; idx < ast_get_length(arguments_node); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg_node = ast_get_item(arguments_node, idx);
    ast_node_t identifier = ast_get_child(arg_node, "identifier");
    ctype_t ctype = array_get(arguments, idx);
    if (!ctype->mut) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, ctype->type, stream);
    stream_write(stream, " ");
    stream_write_location(stream, identifier->loc);
  }
  stream_write(stream, ") {");
  stream_inc_indent(stream);
  write_c_function_body(ctx, body, stream);
  stream_dec_indent(stream);
  stream_newline(stream);
  stream_write(stream, "}");
  stream_newline(stream);
}