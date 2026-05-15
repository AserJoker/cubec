#include "ast/literal_char.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/position.h"
#include "reader/token.h"

ast_node_t read_literal_char(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token || token->type != TOKEN_TYPE_CHAR) {
    return NULL;
  }
  stream->position++;
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_LITERAL_CHAR);
  node->start = position;
  node->end = stream->position;
  return node;
}