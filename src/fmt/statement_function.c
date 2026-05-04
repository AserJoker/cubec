#include "fmt/statement_function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "fmt/function_declarator.h"
void fmt_statement_function(allocator_t allocator, ast_node_t node,
                            stream_t stream) {
  ast_node_t function = ast_get_child(node, "function");
  fmt_function_delcarator(allocator, function, stream);
}