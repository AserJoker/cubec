#include "writer/statement_function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "writer/function_declarator.h"
void write_statement_function(allocator_t allocator, ast_node_t node,
                              stream_t stream) {
  ast_node_t function = ast_get_child(node, "_function");
  write_function_delcarator(allocator, function, stream);
}