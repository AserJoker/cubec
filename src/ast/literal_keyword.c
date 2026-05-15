#include "ast/literal_keyword.h"
#include "reader/token_type.h"

ast_node_t read_literal_keyword(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token || token->type != TOKEN_TYPE_KEYWORD) {
    return NULL;
  }
  stream->position++;
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_LITERAL_KEYWORD);
  node->start = position;
  node->end = stream->position;
  return node;
}