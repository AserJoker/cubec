#include "fmt/statement_import.h"
#include "core/stream.h"
void fmt_statement_import(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t source = ast_get_child(node, "source");
  ast_node_t identifier = ast_get_child(node, "identifier");
  stream_write(stream, "import ");
  stream_write_location(stream, identifier->loc);
  stream_write(stream, " from ");
  stream_write_location(stream, source->loc);
  stream_write(stream, ";");
}