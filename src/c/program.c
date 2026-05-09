#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/statement_declaration.h"
#include "core/stream.h"
#include "engine/context.h"
void write_c_program(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  stream_write(stream, "#include <stdbool.h>\n");
  stream_write(stream, "#include <stdint.h>\n");
  stream_write(stream, "#include <stdlib.h>\n");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      write_c_statement_declaration(ctx, sts, stream);
    }
  }
}