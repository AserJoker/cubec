#include "fmt/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/stream.h"
#include "fmt/expression.h"
#include "fmt/expression_spread.h"
#include "fmt/initialize_field.h"
void fmt_initialize_list(allocator_t allocator, ast_node_t node,
                         stream_t stream) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  stream_write(stream, ".");
  if (type) {
    fmt_expression(allocator, type, stream);
  }
  stream_write(stream, "{");
  if (ast_get_length(fields)) {
    stream_inc_indent(stream);
    stream_newline(stream);
    for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
      if (idx != 0) {
        stream_write(stream, ",");
        stream_newline(stream);
      }
      ast_node_t field = ast_get_item(fields, idx);
      if (field->type == NODE_TYPE_INITIALIZE_FIELD) {
        fmt_initialize_field(allocator, field, stream);
      } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
        fmt_expression_spread(allocator, field, stream);
      } else {
        fmt_expression(allocator, field, stream);
      }
    }
    stream_dec_indent(stream);
    stream_newline(stream);
  }
  stream_write(stream, "}");
}