#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_literal_symbol(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token || token->type != TOKEN_TYPE_SYMBOL) {
    return NULL;
  }
  stream->position++;
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_LITERAL_SYMBOL);
  node->start = position;
  node->end = stream->position;
  return node;
}