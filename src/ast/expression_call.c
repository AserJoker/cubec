#include "ast/expression_call.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_call(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_CALL);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t arguments = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "arguments", arguments);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t argument = read_expression_single(allocator, stream);
      if (!argument) {
        argument = read_expression_spread(allocator, stream);
      }
      if (!argument) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      if (argument->type == NODE_TYPE_ERROR) {
        err = argument;
        goto onerror;
      }
      ast_add_item(arguments, argument);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else if (token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
        break;
      } else {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "missing ','");
        goto onerror;
      }
    }
  }
  stream->position++;
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}