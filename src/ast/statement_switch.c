#include "ast/statement_switch.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/switch_match.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

/**
switch(value) {
[]->{},
}
*/

ast_node_t read_statement_switch(allocator_t allocator, token_stream_t stream) {

  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "switch")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  node = create_ast_node(allocator, NODE_TYPE_STATEMENT_SWITCH);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t condition = read_expression(allocator, stream);
  if (!condition) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "{")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t items = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "items", items);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t item = read_switch_match(allocator, stream);
      if (!item) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      ast_add_item(items, item);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else if (!token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      if (token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
        break;
      }
    }
  }
  stream->position++;

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return NULL;
}