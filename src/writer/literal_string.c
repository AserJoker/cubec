#include "writer/literal_string.h"
#include "core/stream.h"
void write_literal_string(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  stream_write_location(stream, node->loc);
}