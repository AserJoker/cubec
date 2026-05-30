#include "c/statement_declaration.h"
#include "ast/node.h"
#include "c/type.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/stream.h"
#include "engine/context.h"
void c_statement_declaration(c_writer_t writer, ast_node_t node) {
  context_t ctx = writer->ctx;
  ast_node_t kind = ast_get_child(node, "kind");
  if (kind && node_location_is(kind, "comptime")) {
    return;
  }
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t declarations = ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declaration = ast_get_item(declarations, idx);
    ast_node_t identifier = ast_get_child(declaration, "identifier");
    ast_node_t initialize = ast_get_child(declaration, "initialize");
    value_t value = initialize->value;
    char *name = location_get(node_get_location(identifier), ctx->allocator);
    if (ctx->type == CONTEXT_TYPE_STRUCT) {
      c_writer_add_global(writer, name, value);
    } else {
      if (node_location_is(mut, "const")) {
        stream_write(writer->stream, "const ");
      }
      c_type(writer, value->type);
      stream_write(writer->stream, " ");
      stream_write(writer->stream, name);
      // TODO: write initialize
      // stream_write(writer->stream, " = ");
      stream_write(writer->stream, ";");
      stream_newline(writer->stream);
    }
    allocator_free(ctx->allocator, name);
  }
}