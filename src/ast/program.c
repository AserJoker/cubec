#include "ast/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/statement_empty.h"
#include "ast/statement_enum.h"
#include "ast/statement_function.h"
#include "ast/statement_import.h"
#include "ast/statement_struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "reader/token.h"

// [comment]
// [
//   (import|declaration|function|struct|enum)
//   [comment]
// ]*
// [comment]

ast_node_t read_program(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t start = token_stream_get(stream);
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_PROGRAM);
  ast_node_t err = NULL;
  if (!start) {
    return node;
  }
  skip_comments(stream);
  ast_node_t statements = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "statements", statements);
  for (;;) {
    ast_node_t sts = read_statement_import(allocator, stream);
    if (!sts) {
      sts = read_statement_declaration(allocator, stream);
    }
    if (!sts) {
      sts = read_statement_function(allocator, stream);
    }
    if (!sts) {
      sts = read_statement_struct(allocator, stream);
    }
    if (!sts) {
      sts = read_statement_enum(allocator, stream);
    }
    if (!sts) {
      sts = read_statement_empty(allocator, stream);
    }
    if (!sts) {
      break;
    }
    if (sts->type == NODE_TYPE_ERROR) {
      err = sts;
      goto onerror;
    }
    ast_add_item(node, statements);
    skip_comments(stream);
  }
  skip_comments(stream);
  if (stream->position != array_get_size(stream->tokens)) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    return create_ast_error(allocator, start->loc.begin, end->loc.end,
                            stream->filename, "unexpected token");
  }
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  stream->position = position;
  allocator_free(allocator, node);
  return err;
}