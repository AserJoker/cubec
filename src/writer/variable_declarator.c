#include "writer/variable_declarator.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/string.h"
#include "writer/expression.h"
void write_variable_declarator(allocator_t allocator, ast_node_t node,
                               string_t out) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t initialize = ast_get_child(node, "initialize");
  ast_node_t identifier = ast_get_child(node, "identifier");
  string_concat_location(out, allocator, identifier->loc);
  if (type) {
    string_concat(out, allocator, ": ");
    write_expression(allocator, type, out);
  }
  if (initialize) {
    string_concat(out, allocator, " = ");
    write_expression(allocator, initialize, out);
  }
}