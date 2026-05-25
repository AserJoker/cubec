#include "ast/callable_declarator.h"
#include "ast/callable_argument.h"
#include "ast/callable_argument_rest.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_callable_declarator(allocator_t allocator,
                                    token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "func")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  node = create_ast_node(allocator, NODE_TYPE_CALLABLE_DECLARATOR);
  token = token_stream_get(stream);
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
      ast_node_t arg = read_callable_argument(allocator, stream);
      if (!arg) {
        arg = read_callable_argument_rest(allocator, stream);
      }
      if (!arg) {
        goto onerror;
      }
      if (arg->type == NODE_TYPE_ERROR) {
        err = arg;
        goto onerror;
      }
      ast_add_item(arguments, arg);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
        break;
      } else if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else {
        goto onerror;
      }
    }
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "->")) {
    goto onerror;
  }
  stream->position++;
  ast_node_t type = read_expression_value(allocator, stream);
  if (!type) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}