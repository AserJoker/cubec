#include "writer/literal_numeric.h"
#include "core/stream.h"
void write_literal_numeric(allocator_t allocator, ast_node_t node,
                           stream_t stream) {
  stream_write_location(stream, node->loc);
}