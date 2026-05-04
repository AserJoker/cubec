#include "fmt/statement_struct.h"
#include "core/stream.h"
#include "fmt/struct_declarator.h"

void fmt_statement_struct(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t stru = ast_get_child(node, "struct");
  fmt_struct_declarator(allocator, stru, stream);
}