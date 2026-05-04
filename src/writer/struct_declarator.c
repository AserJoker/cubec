#include "writer/struct_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/stream.h"
#include "writer/expression.h"
#include "writer/expression_spread.h"
#include "writer/statement_declaration.h"
#include "writer/statement_function.h"
#include "writer/statement_struct.h"

void write_struct_declarator(allocator_t allocator, ast_node_t node,
                             stream_t stream) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t fields = ast_get_child(node, "fields");
  ast_node_t decorators = ast_get_child(node, "decorators");
  ast_node_t pub = ast_get_child(node, "pub");
  ast_node_t aligned = ast_get_child(node, "aligned");
  ast_node_t packed = ast_get_child(node, "packed");
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t dec = ast_get_item(decorators, idx);
    ast_node_t expr = ast_get_child(dec, "expression");
    stream_write(stream, "[[");
    write_expression(allocator, expr, stream);
    stream_write(stream, "]]");
    stream_newline(stream);
  }
  if (pub) {
    stream_write_location(stream, pub->loc);
    stream_write(stream, " ");
  }
  stream_write(stream, "struct ");
  if (packed) {
    stream_write(stream, "packed ");
  }
  if (aligned) {
    stream_write(stream, "aligned(");
    stream_write_location(stream, aligned->loc);
    stream_write(stream, ") ");
  }
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
      if (field->visible) {
        if (count != 0) {
          stream_newline(stream);
        }
        if (field->type == NODE_TYPE_STRUCT_FIELD) {
          ast_node_t decorators = ast_get_child(field, "decorators");
          if (decorators) {
            for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
              ast_node_t dec = ast_get_item(decorators, idx);
              ast_node_t expr = ast_get_child(dec, "expression");
              stream_write(stream, "[[");
              write_expression(allocator, expr, stream);
              stream_write(stream, "]]");
              stream_newline(stream);
            }
          }
          ast_node_t identifier = ast_get_child(field, "identifier");
          ast_node_t type = ast_get_child(field, "type");
          ast_node_t pub = ast_get_child(field, "pub");
          ast_node_t mut = ast_get_child(field, "mut");
          if (pub) {
            stream_write_location(stream, pub->loc);
            stream_write(stream, " ");
          }
          stream_write_location(stream, identifier->loc);
          stream_write(stream, ": ");
          if (mut) {
            stream_write_location(stream, mut->loc);
            stream_write(stream, " ");
          }
          write_expression(allocator, type, stream);
          stream_write(stream, ";");
        } else if (field->type == NODE_TYPE_STATEMENT_STRUCT) {
          write_statement_struct(allocator, field, stream);
        } else if (field->type == NODE_TYPE_STATEMENT_FUNCTION) {
          write_statement_function(allocator, field, stream);
        } else if (field->type == NODE_TYPE_STATEMENT_DECLARATION) {
          write_statement_declaration(allocator, field, stream);
        } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
          write_expression_spread(allocator, field, stream);
          stream_write(stream, ";");
        }
        count++;
      }
    }
    stream_dec_indent(stream);
    stream_newline(stream);
  }
  stream_write(stream, "}");
}