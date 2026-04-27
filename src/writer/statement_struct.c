#include "writer/statement_struct.h"
#include "core/stream.h"
#include "writer/struct_declarator.h"

void write_statement_struct(allocator_t allocator, ast_node_t node,
                            stream_t stream) {
  ast_node_t stru = ast_get_child(node, "struct");
  ast_node_t pub = ast_get_child(node, "pub");
  if (pub) {
    stream_write_location(stream, pub->loc);
    stream_write(stream, " ");
  }
  write_struct_declarator(allocator, stru, stream);
}