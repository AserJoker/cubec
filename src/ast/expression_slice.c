#include "ast/expression_slice.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_slice(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_SLICE);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "[")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t start = read_expression(allocator, stream);
  if (start) {
    if (start->type == NODE_TYPE_ERROR) {
      err = start;
      goto onerror;
    }
    ast_add_child(allocator, node, "start", start);
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ":")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t end = read_expression(allocator, stream);
  if (end) {
    if (end->type == NODE_TYPE_ERROR) {
      err = end;
      goto onerror;
    }
    ast_add_child(allocator, node, "end", end);
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "]")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ']'");
    goto onerror;
  }
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}