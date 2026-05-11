#include "c/function_body.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/statement_declaration.h"
#include "c/statement_return.h"
#include "core/stream.h"

void write_c_function_body(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->visible) {
      if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
        stream_newline(stream);
        write_c_statement_declaration(ctx, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_RETURN) {
        stream_newline(stream);
        write_c_statement_return(ctx, sts, stream);
      }
    }
  }
}