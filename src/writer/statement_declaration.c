#include "writer/statement_declaration.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#include "writer/variable_declarator.h"

void write_statement_declaration(allocator_t allocator, ast_node_t node,
                                 stream_t stream) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t declarations = ast_get_child(node, "declarations");
  if (kind) {
    stream_write_location(stream, kind->loc);
    stream_write(stream, " ");
  }
  stream_write_location(stream, type->loc);
  stream_write(stream, " ");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    ast_node_t declar = ast_get_item(declarations, idx);
    write_variable_declarator(allocator, declar, stream);
  }
  stream_write(stream, ";");
}