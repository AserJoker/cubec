#include "writer/literal_identifier.h"
#include "core/string.h"
void write_literal_identifier(allocator_t allocator, ast_node_t node,
                              string_t out) {
  string_concat_location(out, allocator, node->loc);
}