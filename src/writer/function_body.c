#include "writer/function_body.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/stream.h"
#include "writer/statement_declaration.h"
#include "writer/statement_function.h"
#include "writer/statement_return.h"
#include "writer/statement_struct.h"

void write_function_body(allocator_t allocator, ast_node_t node,
                         stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  stream_write(stream, " {");
  stream_inc_indent(stream);
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->visible) {
      stream_newline(stream);
      if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
        write_statement_declaration(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
        write_statement_function(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
        write_statement_return(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
        write_statement_struct(allocator, sts, stream);
      }
    }
  }
  stream_dec_indent(stream);
  stream_newline(stream);
  stream_write(stream, "}");
}