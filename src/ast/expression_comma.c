#include "ast/expression_comma.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_comma(allocator_t allocator, token_stream_t stream) {
  size_t position = stream->position;
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  ast_node_t left = read_expression_single(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
    return left;
  }
  stream->position++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_COMMON);
  ast_add_child(allocator, node, "left", left);
  skip_comments(stream);
  ast_node_t right = read_expression_comma(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}