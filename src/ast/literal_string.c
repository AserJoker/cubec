#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"
#include <stdint.h>

ast_node_t read_literal_string(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token || token->type != TOKEN_TYPE_STRING) {
    return NULL;
  }
  stream->position++;
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_LITERAL_STRING);
  node->start = position;
  node->end = stream->position;
  return node;
}