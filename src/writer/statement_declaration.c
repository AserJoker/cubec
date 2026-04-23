#include "writer/statement_declaration.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/string.h"
#include "writer/variable_declarator.h"

void write_statement_declaration(allocator_t allocator, ast_node_t node,
                                 string_t out) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t declarations = ast_get_child(node, "declarations");
  if (kind) {
    string_concat_location(out, allocator, kind->loc);
    string_concat(out, allocator, " ");
  }
  string_concat_location(out, allocator, type->loc);
  string_concat(out, allocator, " ");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    if (idx != 0) {
      string_concat(out, allocator, ", ");
    }
    ast_node_t declar = ast_get_item(declarations, idx);
    write_variable_declarator(allocator, declar, out);
  }
  string_concat(out, allocator, ";");
}