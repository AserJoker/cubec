#include "writer/value.h"
#include "core/allocator.h"
#include "engine/value.h"
void write_value(allocator_t allocator, ast_node_t node, stream_t stream) {
  char *str = value_write_ast(node->value, allocator);
  stream_write(stream, str);
  allocator_free(allocator, str);
}