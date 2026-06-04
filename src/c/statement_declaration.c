#include "c/statement_declaration.h"
#include "ast/node.h"
#include "c/expression.h"
#include "c/type.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/stream.h"
void c_statement_declaration(c_writer_t writer, ast_node_t node) {
  context_t ctx = writer->ctx;
  stream_t stream = writer->stream;
  allocator_t allocator = ctx->allocator;
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t declarations = ast_get_child(node, "declarations");
  if (kind && node_location_is(kind, "comptime")) {
    return;
  }
  bool is_mut = node_location_is(mut, "let");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declar = ast_get_item(declarations, idx);
    ast_node_t type = ast_get_child(declar, "type");
    ast_node_t identifier = ast_get_child(declar, "identifier");
    ast_node_t initialize = ast_get_child(declar, "initialize");
    if (!is_mut) {
      stream_write(stream, "const ");
    }
    type_t t = *(type_t *)type->value->data;
    c_type(writer, t);
    char *id = location_get(node_get_location(identifier), allocator);
    stream_write(stream, " %s = ", id);
    allocator_free(allocator, id);
    c_expression(writer, initialize);
    stream_write(stream, ";");
    stream_newline(stream);
  }
}