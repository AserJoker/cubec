#include "fmt/literal_identifier.h"
void fmt_literal_identifier(allocator_t allocator, ast_node_t node,
                            stream_t stream) {
  stream_write_location(stream, node->loc);
}