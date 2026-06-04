#include "c/statement_block.h"
#include "ast/node.h"
#include "c/statement.h"
#include "core/stream.h"

void c_statement_block(c_writer_t writer, ast_node_t node) {
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  stream_t stream = writer->stream;
  scope_t scope = ctx->current;
  ctx->current = node->scope;
  ast_node_t statements = ast_get_child(node, "statements");
  stream_write(stream, "{");
  if (ast_get_length(statements)) {
    stream_inc_indent(stream);
    stream_newline(stream);
    for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
      ast_node_t sts = ast_get_item(statements, idx);
      c_statement(writer, sts);
    }
    stream_dec_indent(stream);
    stream_newline(stream);
  }
  stream_write(stream, "}");
  stream_newline(stream);
  ctx->current = scope;
}