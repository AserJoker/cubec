#include "ast/expression_group.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"
ast_node_t read_expression_group(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    goto onerror;
  }
  stream->position++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_GROUP);
  skip_comments(stream);
  ast_node_t expression = read_expression(allocator, stream);
  if (!expression) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (expression->type == NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", expression);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ')'");
    goto onerror;
  }
  stream->position++;

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  stream->position = position;
  allocator_free(allocator, node);
  return err;
}
ast_node_t ast_unwrap_group(ast_node_t node) {
  while (node->type == NODE_TYPE_EXPRESSION_GROUP) {
    node = ast_get_child(node, "expression");
  }
  return node;
}