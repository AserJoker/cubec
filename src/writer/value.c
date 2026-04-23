#include "writer/value.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/value.h"
void write_value(allocator_t allocator, ast_node_t node, string_t out) {
  char *str = value_write_ast(node->value, allocator);
  string_concat(out, allocator, str);
  allocator_free(allocator, str);
}