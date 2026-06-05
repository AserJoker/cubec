#include "c/function_declarator.h"
#include "ast/node.h"
#include "c/function.h"
void c_function_declarator(c_writer_t writer, ast_node_t node) {
  ast_node_t bind = ast_get_child(node, "bind");
  c_function_closure(writer, bind->value);
}
