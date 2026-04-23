#include "writer/expression_binary.h"
#include "core/string.h"
#include "writer/expression.h"
void write_expression_binary(allocator_t allocator, ast_node_t node,
                             string_t out) {
  ast_node_t left_node = ast_get_child(node, "left");
  ast_node_t right_node = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (left_node) {
    write_expression(allocator, left_node, out);
    string_concat(out, allocator, " ");
  }
  string_concat_location(out, allocator, opt->loc);
  if (left_node) {
    string_concat(out, allocator, " ");
  }
  write_expression(allocator, right_node, out);
}