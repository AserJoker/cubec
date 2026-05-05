#include "fmt/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/stream.h"
#include "fmt/expression.h"
#include "fmt/function_argument.h"
#include "fmt/function_argument_rest.h"
#include "fmt/function_body.h"
void fmt_function_delcarator(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t body = ast_get_child(node, "body");
  ast_node_t decorators = ast_get_child(node, "decorators");
  ast_node_t closure = ast_get_child(node, "closure");
  ast_node_t pub_node = ast_get_child(node, "pub");
  if (decorators) {
    for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
      ast_node_t dec = ast_get_item(decorators, idx);
      ast_node_t expr = ast_get_child(dec, "expression");
      stream_write(stream, "[[");
      fmt_expression(allocator, expr, stream);
      stream_write(stream, "]]");
      stream_newline(stream);
    }
  }
  if (pub_node) {
    stream_write_location(stream, pub_node->loc);
    stream_write(stream, " ");
  }
  if (kind) {
    stream_write_location(stream, kind->loc);
    stream_write(stream, " ");
  }
  stream_write(stream, "func");
  if (ast_get_length(closure)) {
    stream_write(stream, " [");
    for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
      if (idx != 0) {
        stream_write(stream, ", ");
      }
      ast_node_t item = ast_get_item(closure, idx);
      char *name = location_get(item->loc, allocator);
      stream_write(stream, name);
      allocator_free(allocator, name);
    }
    stream_write(stream, "]");
  }
  if (identifier) {
    stream_write(stream, " ");
    stream_write_location(stream, identifier->loc);
  }
  stream_write(stream, "(");
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t arg = ast_get_item(arguments, idx);
    if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT) {
      fmt_function_argument(allocator, arg, stream);
    } else if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      fmt_function_argument_rest(allocator, arg, stream);
    }
  }
  stream_write(stream, "): ");
  fmt_expression(allocator, type, stream);
  if (body) {
    fmt_function_body(allocator, body, stream);
  } else {
    stream_write(stream, ";");
  }
}