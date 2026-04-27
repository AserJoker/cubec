#include "writer/struct_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/stream.h"
#include "writer/expression.h"
#include "writer/function_declarator.h"
#include "writer/statement_declaration.h"

void write_struct_declarator(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t fields = ast_get_child(node, "fields");
  stream_write(stream, "struct ");
  if (identifier) {
    stream_write_location(stream, identifier->loc);
    stream_write(stream, " ");
  }
  stream_write(stream, "{");
  size_t count = 0;
  if (ast_get_length(fields)) {
    stream_inc_indent(stream);
    stream_newline(stream);
    for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
      ast_node_t field = ast_get_item(fields, idx);
      if (count != 0) {
        stream_newline(stream);
      }
      if (field->visible) {
        if (field->type == NODE_TYPE_STRUCT_FIELD) {
          ast_node_t identifier = ast_get_child(field, "identifier");
          ast_node_t type = ast_get_child(field, "type");
          stream_write_location(stream, identifier->loc);
          stream_write(stream, ": ");
          write_expression(allocator, type, stream);
          stream_write(stream, ";");
        } else if (field->type == NODE_TYPE_STRUCT_DECLARATOR) {
          write_struct_declarator(allocator, field, stream);
        } else if (field->type == NODE_TYPE_FUNCTION_DECLARATOR) {
          write_function_delcarator(allocator, field, stream);
        } else if (field->type == NODE_TYPE_STATEMENT_DECLARATION) {
          write_statement_declaration(allocator, field, stream);
        }
        count++;
      }
    }
    stream_dec_indent(stream);
    stream_newline(stream);
  }
  stream_write(stream, "}");
}