#include "ast/literal_numeric.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"

ast_node_t read_literal_numeric(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token || token->type != TOKEN_TYPE_NUMERIC) {
    return NULL;
  }
  stream->position++;
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_LITERAL_NUMERIC);

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
}